#include "write_application_layer.h"
#include "write_data_layer.c"

extern int sumAlarms ;
extern int flagAlarm;
 unsigned char nMsgs = 0;
 int nFrames = 0;
extern  struct termios oldtio, newtio;

// handler do sinal de alarme
void alarmHandler() {
  printf("Alarm=%d\n", sumAlarms + 1);
  flagAlarm = TRUE;
  sumAlarms++;
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


unsigned char *headerAL(unsigned char *mensagem, off_t fileSize_bytes,
                        int *sizePacket) {
  unsigned char *mensagemFinal = (unsigned char *)malloc(fileSize_bytes + 4);
  mensagemFinal[0] = headerC;
  mensagemFinal[1] = nMsgs % 255;
  mensagemFinal[2] = (int)fileSize_bytes / 256;
  mensagemFinal[3] = (int)fileSize_bytes % 256;
  memcpy(mensagemFinal + 4, mensagem, *sizePacket);
  *sizePacket += 4;
  nMsgs++;
  nFrames++;
  return mensagemFinal;
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


int main(int argc, char **argv) {
  // int c, res;
  // char buf[255];
  int fd;
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

  fd = open(argv[1], O_RDWR | O_NOCTTY);
  if (fd < 0) {
    perror(argv[1]);
    exit(-1);
  }

  if (tcgetattr(fd, &oldtio) ==
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
  if (!LLOPEN(fd, 0)) {
    return -1;
  }

  int sizeOfFileName = strlen(argv[2]);
  unsigned char *fileName = (unsigned char *)malloc(sizeOfFileName);
  fileName = (unsigned char *)argv[2];
  unsigned char *start = controlPackageI(C2Start, fileSize_bytes, fileName,
                                         sizeOfFileName, &sizeControlPackageI);

  LLWRITE(fd, start, sizeControlPackageI);
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
    if (!LLWRITE(fd, mensagemHeader, headerSize)) {
      printf("Limite de alarmes atingido\n");
      return -1;
    }
  }

  unsigned char *end = controlPackageI(C2End, fileSize_bytes, fileName,
                                       sizeOfFileName, &sizeControlPackageI);
  LLWRITE(fd, end, sizeControlPackageI);
  printf("Mandou frame END\n");

  LLCLOSE(fd);

  // fim do relógio
  clock_gettime(0, &requestEnd);

  double accum = (requestEnd.tv_sec - requestStart.tv_sec) +
                 (requestEnd.tv_nsec - requestStart.tv_nsec) / 1E9;

  printf("Seconds passed: %f\n", accum);

  sleep(1);

  close(fd);
  return 0;
}