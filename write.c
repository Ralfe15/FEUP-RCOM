
#include "write.h"
#include "stateMachine.c"

unsigned char SET[5];
unsigned char UA[5];

volatile int STOP = FALSE;
int DONE = FALSE;
int attempts = 0;
int fileDescriptor; // serial file descriptor
int sumAlarms = 0;
int flagAlarm = FALSE;
int frame = 0;
unsigned char nMsgs = 0;
int nFrames = 0;
struct termios oldtio, newtio;

void prepareSet() {
  SET[0] = FLAG;
  SET[1] = A;
  SET[2] = C_SET;
  SET[3] = SET[1] ^ SET[2];
  SET[4] = FLAG;
}
void prepareUA() {
  UA[0] = FLAG;
  UA[1] = A;
  UA[2] = C_UA;
  UA[3] = UA[1] ^ UA[2];
  UA[4] = FLAG;
}
// handler do sinal de alarme
void alarmHandler() {
  printf("Alarm=%d\n", sumAlarms + 1);
  flagAlarm = TRUE;
  sumAlarms++;
}
void sendFrame() {
  for (; attempts <= 3; attempts++) {
    printf("Attempt %d/3:\n", (attempts + 1));
    int sentBytes = 0;
    while (sentBytes != 5) {
      sentBytes = write(fileDescriptor, SET, 5);
      printf("Set Flag sended, %d sentBytes \n", sentBytes);
    }
    if (attempts == 3) {
      printf("Failed 3 times, exiting...\n");
      exit(-1);
    }
  }
  if (!DONE) {
    alarm(3);
  }
}

void receiveFrame(int fileDescriptor) {
  unsigned char c; // last char received
  int state = 0;
  printf("Receiving UA...\n");
  while (state != 5) {
    read(fileDescriptor, &c, 1);
    printf("State %d - char: 0x%X\n", state, c);
    switch (state) {
    case 0: // expecting flag
      if (c == UA[0]) {
        state = 1;
      } // else stay in same state
      break;
    case 1: // expecting A
      if (c == UA[1]) {
        state = 2;
      } else if (c != UA[0]) { // if not FLAG instead of A
        state = 0;
      } // else stay in same state
      break;
    case 2: // Expecting C_SET
      if (c == UA[2]) {
        state = 3;
      } else if (c == UA[0]) { // if FLAG received
        state = 1;
      } else { // else go back to beggining
        state = 0;
      }
      break;
    case 3: // Expecting BCC
      if (c == UA[3]) {
        state = 4;
      } else {
        state = 0; // else go back to beggining
      }
      break;
    case 4: // Expecting FLAG
      if (c == UA[4]) {
        state = 5;
      } else {
        state = 0; // else go back to beggining
      }
      break;
    }
  }
  DONE = TRUE;
  alarm(0);
  printf("Received UA properly\n");
}
unsigned char *headerAL(unsigned char *mensagem, off_t fileSize_bytes,
                        int *sizePacket) {
  unsigned char *buffer_final = (unsigned char *)malloc(fileSize_bytes + 4);
  buffer_final[0] = headerC;
  buffer_final[1] = nMsgs % 255;
  buffer_final[2] = (int)fileSize_bytes / 256;
  buffer_final[3] = (int)fileSize_bytes % 256;
  memcpy(buffer_final + 4, mensagem, *sizePacket);
  *sizePacket += 4;
  nMsgs++;
  nFrames++;
  return buffer_final;
}

unsigned char *splitMessage(unsigned char *mensagem, off_t *index,
                            int *sizePacket, off_t fileSize_bytes) {
  unsigned char *packet;
  int i = 0;
  off_t j = *index;
  if (*index + *sizePacket > fileSize_bytes) {
    *sizePacket = fileSize_bytes - *index;
  }
  packet = (unsigned char *)malloc(*sizePacket);
  for (; i < *sizePacket; i++, j++) {
    packet[i] = mensagem[j];
  }
  *index = j;
  return packet;
}
// -------------------------------------------------------------
// LL OPEN
// -------------------------------------------------------------
int LLOPEN(int fd, int x) {

  if (tcgetattr(fd, &oldtio) == -1) {
    perror("tcgetattr");
    exit(-1);
  }
  // Clear struct for new port settings
  memset(&newtio, 0, sizeof(newtio));

  bzero(&newtio, sizeof(newtio));
  newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;

  // Set input mode (non-canonical, no echo,...)
  newtio.c_lflag = 0;

  newtio.c_cc[VTIME] = 1; /* inter-unsigned character timer unused */
  newtio.c_cc[VMIN] = 0;  /* blocking read until 5 unsigned chars received */

  /*
  VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
  leitura do(s) pr�ximo(s) caracter(es)
  */

  tcflush(fd, TCIOFLUSH);

  if (tcsetattr(fd, TCSANOW, &newtio) == -1) {
    perror("tcsetattr");
    exit(-1);
  }

  printf("New termios structure set\n");

  unsigned char c;
  do {
    sendControlMessage(fd, SET_C);
    alarm(TIMEOUT);
    flagAlarm = 0;
    int state = 0;

    while (!DONE && !flagAlarm) {
      read(fd, &c, 1);
      startVerifyState(&c,&state, &DONE);
    }
  } while (flagAlarm && sumAlarms < NUMMAX);
  printf("flag alarm %d\n", flagAlarm);
  printf("Total of Alarms %d\n", sumAlarms);
  if (flagAlarm && sumAlarms == 3) {
    return FALSE;
  } else {
    flagAlarm = FALSE;
    sumAlarms = 0;
    return TRUE;
  }
}

// -------------------------------------------------------------
// LL WRITE
// -------------------------------------------------------------
int LLWRITE(int fd, const unsigned char *buffer, int buf_size) {
  unsigned char BCC2;
  unsigned char *BCC2Stuffed = (unsigned char *)malloc(sizeof(unsigned char));
  unsigned char *buffer_final =
      (unsigned char *)malloc((buf_size + 6) * sizeof(unsigned char));
  int buffer_final_size = buf_size + 6;
  int sizeBCC2 = 1;
  BCC2 = calculoBCC2(buffer, buf_size);
  BCC2Stuffed = stuffingBCC2(BCC2, &sizeBCC2);
  int rejeitado = FALSE;

  buffer_final[0] = FLAG;
  buffer_final[1] = A;
  if (frame == 0) {
    buffer_final[2] = C10;
  } else {
    buffer_final[2] = C11;
  }
  buffer_final[3] = (buffer_final[1] ^ buffer_final[2]);

  int i = 0;
  int j = 4;
  for (; i < buf_size; i++) {
    if (buffer[i] == FLAG) {
      buffer_final =
          (unsigned char *)realloc(buffer_final, ++buffer_final_size);
      buffer_final[j] = Escape;
      buffer_final[j + 1] = escapeFlag;
      j = j + 2;
    } else {
      if (buffer[i] == Escape) {
        buffer_final =
            (unsigned char *)realloc(buffer_final, ++buffer_final_size);
        buffer_final[j] = Escape;
        buffer_final[j + 1] = escapeEscape;
        j = j + 2;
      } else {
        buffer_final[j] = buffer[i];
        j++;
      }
    }
  }

  if (sizeBCC2 == 1)
    buffer_final[j] = BCC2;
  else {
    buffer_final =
        (unsigned char *)realloc(buffer_final, ++buffer_final_size);
    buffer_final[j] = BCC2Stuffed[0];
    buffer_final[j + 1] = BCC2Stuffed[1];
    j++;
  }
  buffer_final[j + 1] = FLAG;

  // mandar mensagem
  do {

    unsigned char *copia;
    copia = messUpBCC1(buffer_final, buffer_final); // altera bcc1
    copia = messUpBCC2(copia, buffer_final);         // altera bcc2
    write(fd, copia, buffer_final);

    flagAlarm = FALSE;
    alarm(TIMEOUT);
    unsigned char C = readControlMessageC(fd);
    if ((C == CRR1 && frame == 0) || (C == CRR0 && frame == 1)) {
      printf("Recebeu rr %x, frame = %d\n", C, frame);
      rejeitado = FALSE;
      sumAlarms = 0;
      frame ^= 1;
      alarm(0);
    } else {
      if (C == CREJ1 || C == CREJ0) {
        rejeitado = TRUE;
        printf("Recebeu rej %x, frame=%d\n", C, frame);
        alarm(0);
      }
    }
  } while ((flagAlarm && sumAlarms < NUMMAX) || rejeitado);
  if (sumAlarms >= NUMMAX)
    return FALSE;
  else
    return TRUE;
}
unsigned char *messUpBCC2(unsigned char *packet, int sizePacket) {
  unsigned char *copia = (unsigned char *)malloc(sizePacket);
  memcpy(copia, packet, sizePacket);
  int r = (rand() % 100) + 1;
  if (r <= bcc2ErrorPercentage) {
    int i = (rand() % (sizePacket - 5)) + 4;
    unsigned char randomLetter = (unsigned char)('A' + (rand() % 26));
    copia[i] = randomLetter;
    printf("Modifiquei BCC2\n");
  }
  return copia;
}

unsigned char *messUpBCC1(unsigned char *packet, int sizePacket) {
  unsigned char *copia = (unsigned char *)malloc(sizePacket);
  memcpy(copia, packet, sizePacket);
  int r = (rand() % 100) + 1;
  if (r <= bcc1ErrorPercentage) {
    int i = (rand() % 3) + 1;
    unsigned char randomLetter = (unsigned char)('A' + (rand() % 26));
    copia[i] = randomLetter;
    printf("Modifiquei BCC1\n");
  }
  return copia;
}
/////////////////////////////

void sendControlMessage(int fd, unsigned char C) {
  unsigned char message[5];
  message[0] = FLAG;
  message[1] = A;
  message[2] = C;
  message[3] = message[1] ^ message[2];
  message[4] = FLAG;
  write(fd, message, 5);
}

unsigned char readControlMessageC(int fd) {
  int state = 0;
  unsigned char c;
  unsigned char C;

  while (!flagAlarm && state != 5) {
    read(fd, &c, 1);
    switch (state) {
    // recebe FLAG
    case 0:
      if (c == FLAG)
        state = 1;
      break;
    // recebe A
    case 1:
      if (c == A)
        state = 2;
      else {
        if (c == FLAG)
          state = 1;
        else
          state = 0;
      }
      break;
    // recebe c
    case 2:
      if (c == CRR0 || c == CRR1 || c == CREJ0 || c == CREJ1 || c == DISC) {
        C = c;
        state = 3;
      } else {
        if (c == FLAG)
          state = 1;
        else
          state = 0;
      }
      break;
    // recebe BCC
    case 3:
      if (c == (A ^ C))
        state = 4;
      else
        state = 0;
      break;
    // recebe FLAG final
    case 4:
      if (c == FLAG) {
        alarm(0);
        state = 5;
        return C;
      } else
        state = 0;
      break;
    }
  }
  return 0xFF;
}

unsigned char calculoBCC2(unsigned char *mensagem, int size) {
  unsigned char BCC2 = mensagem[0];
  int i;
  for (i = 1; i < size; i++) {
    BCC2 ^= mensagem[i];
  }
  return BCC2;
}

unsigned char *stuffingBCC2(unsigned char BCC2, int *sizeBCC2) {
  unsigned char *BCC2Stuffed;
  if (BCC2 == FLAG) {
    BCC2Stuffed = (unsigned char *)malloc(2 * sizeof(unsigned char *));
    BCC2Stuffed[0] = Escape;
    BCC2Stuffed[1] = escapeFlag;
    (*sizeBCC2)++;
  } else {
    if (BCC2 == Escape) {
      BCC2Stuffed = (unsigned char *)malloc(2 * sizeof(unsigned char *));
      BCC2Stuffed[0] = Escape;
      BCC2Stuffed[1] = escapeEscape;
      (*sizeBCC2)++;
    }
  }

  return BCC2Stuffed;
}

unsigned char *openReadFile(unsigned char *fileName, off_t *fileSize_bytes) {
  FILE *f;
  struct stat metadata;
  unsigned char *fileData;

  if ((f = fopen((char *)fileName, "rb")) == NULL) {
    perror("error opening file!");
    exit(-1);
  }
  stat((char *)fileName, &metadata);
  (*fileSize_bytes) = metadata.st_size;
  printf("This file has %ld bytes \n", *fileSize_bytes);

  fileData = (unsigned char *)malloc(*fileSize_bytes);

  fread(fileData, sizeof(unsigned char), *fileSize_bytes, f);
  return fileData;
}

unsigned char *controlPackageI(unsigned char state, off_t fileSize_bytes,
                               unsigned char *fileName, int sizeOfFileName,
                               int *sizeControlPackageI) {
  *sizeControlPackageI = 9 * sizeof(unsigned char) + sizeOfFileName;
  unsigned char *package = (unsigned char *)malloc(*sizeControlPackageI);

  if (state == C2Start)
    package[0] = C2Start;
  else
    package[0] = C2End;
  package[1] = T1;
  package[2] = L1;
  package[3] = (fileSize_bytes >> 24) & 0xFF;
  package[4] = (fileSize_bytes >> 16) & 0xFF;
  package[5] = (fileSize_bytes >> 8) & 0xFF;
  package[6] = fileSize_bytes & 0xFF;
  package[7] = T2;
  package[8] = sizeOfFileName;
  int k = 0;
  for (; k < sizeOfFileName; k++) {
    package[9 + k] = fileName[k];
  }
  return package;
}


/////////////////////777777
// -------------------------------------------------------------
// LL READ
// -------------------------------------------------------------
int LLREAD(int fd, int x) {
  return 0;
} // -------------------------------------------------------------
// LL CLOSE
// -------------------------------------------------------------
int LLCLOSE(int fd) {
  sendControlMessage(fd, DISC);
  printf("Mandou DISC\n");
  unsigned char C;
  // espera ler o DISC
  C = readControlMessageC(fd);
  while (C != DISC) {
    C = readControlMessageC(fd);
  }
  printf("Leu DISC\n");
  sendControlMessage(fd, UA_C);
  printf("Mandou UA final\n");
  printf("Writer terminated \n");
  if (tcsetattr(fileDescriptor, TCSANOW, &oldtio) == -1) {
    perror("tcsetattr");
    return -1;
  }

  close(fileDescriptor);

  return 1;
}

int main(int argc, char **argv) {
  // int c, res;
  // char buf[255];
  off_t fileSize_bytes; // file size
  off_t index = 0;
  int sizeControlPackageI = 0;

  if ((argc < 3) || ((strcmp("/dev/ttyS10", argv[1]) != 0) &&
                     (strcmp("/dev/ttyS11", argv[1]) != 0))) {
    exit(1);
  }
  /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  */

  fileDescriptor = open(argv[1], O_RDWR | O_NOCTTY);
  if (fileDescriptor < 0) {
    perror(argv[1]);
    exit(-1);
  }

  if (tcgetattr(fileDescriptor, &oldtio) ==
      -1) { /* save current port settings */
    perror("tcgetattr");
    exit(-1);
  }
  // instalar handler do alarme
  (void)signal(SIGALRM, alarmHandler);

  unsigned char *mensagem =
      openReadFile((unsigned char *)argv[2], &fileSize_bytes);

  // inicio do relógio
  struct timespec requestStart, requestEnd;
  clock_gettime(0, &requestStart);

  // se nao conseguirmos efetuar a ligaçao atraves do set e do ua o programa
  // termina
  if (!LLOPEN(fileDescriptor, 0)) {
    return -1;
  }

  int sizeOfFileName = strlen(argv[2]);
  unsigned char *fileName = (unsigned char *)malloc(sizeOfFileName);
  fileName = (unsigned char *)argv[2];
  unsigned char *start = controlPackageI(C2Start, fileSize_bytes, fileName,
                                         sizeOfFileName, &sizeControlPackageI);

  LLWRITE(fileDescriptor, start, sizeControlPackageI);
  printf("Mandou frame START\n");

  int sizePacket = sizePacketConst;
  srand(time(NULL));

  while (sizePacket == sizePacketConst && index < fileSize_bytes) {
    // split mensagem
    unsigned char *packet =
        splitMessage(mensagem, &index, &sizePacket, fileSize_bytes);
    printf("Mandou packet numero %d\n", nFrames);
    // header nivel aplicação
    int headerSize = sizePacket;
    unsigned char *mensagemHeader =
        headerAL(packet, fileSize_bytes, &headerSize);
    // envia a mensagem
    if (!LLWRITE(fileDescriptor, mensagemHeader, headerSize)) {
      printf("Limite de alarmes atingido\n");
      return -1;
    }
  }

  unsigned char *end = controlPackageI(C2End, fileSize_bytes, fileName,
                                       sizeOfFileName, &sizeControlPackageI);
  LLWRITE(fileDescriptor, end, sizeControlPackageI);
  printf("Mandou frame END\n");

  LLCLOSE(fileDescriptor);

  // fim do relógio
  clock_gettime(0, &requestEnd);

  double accum = (requestEnd.tv_sec - requestStart.tv_sec) +
                 (requestEnd.tv_nsec - requestStart.tv_nsec) / 1E9;

  printf("Seconds passed: %f\n", accum);

  sleep(1);

  close(fileDescriptor);
  return 0;
}
