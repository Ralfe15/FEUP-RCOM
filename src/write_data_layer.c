#include "write_data_layer.h"

unsigned char SET[5];
unsigned char UA[5];
int sumAlarms = 0;
int flagAlarm = FALSE;
int frame = 0;
int STOP = FALSE;
int DONE = FALSE;
struct termios oldtio, newtio;


void stateMachineUA(int *state, unsigned char *c) {

  switch (*state) {
  case 0:
    if (*c == FLAG)
      *state = 1;
    break;
  case 1:
    if (*c == A)
      *state = 2;
    else {
      if (*c == FLAG)
        *state = 1;
      else
        *state = 0;
    }
    break;
  case 2:
    if (*c == UA_C)
      *state = 3;
    else {
      if (*c == FLAG)
        *state = 1;
      else
        *state = 0;
    }
    break;
  case 3:
    if (*c == UA_BCC)
      *state = 4;
    else
      *state = 0;
    break;
  case 4:
    if (*c == FLAG) {
      STOP = TRUE;
      alarm(0);
      printf("Received UA\n");
    } else
      *state = 0;
    break;
  }
}


unsigned char readControlMessageC(int fd) {
  int state = 0;
  unsigned char c;
  unsigned char C;

  while (!flagAlarm && state != 5) {
    read(fd, &c, 1);
    switch (state) {
    case 0:
      if (c == FLAG)
        state = 1;
      break;
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
    case 3:
      if (c == (A ^ C))
        state = 4;
      else
        state = 0;
      break;
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


void sendControlMessage(int fd, unsigned char C) {
  unsigned char message[5];
  message[0] = FLAG;
  message[1] = A;
  message[2] = C;
  message[3] = message[1] ^ message[2];
  message[4] = FLAG;
  write(fd, message, 5);
}


unsigned char calculoBCC2(unsigned char *message, int size) {
  unsigned char BCC2 = message[0];
  int i;
  for (i = 1; i < size; i++) {
    BCC2 ^= message[i];
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


unsigned char *messUpBCC1(unsigned char *packet, int sizePacket) {
  unsigned char *copy = (unsigned char *)malloc(sizePacket);
  memcpy(copy, packet, sizePacket);
  int r = (rand() % 100) + 1;
  if (r <= bcc1ErrorPercentage) {
    int i = (rand() % 3) + 1;
    unsigned char randomLetter = (unsigned char)('A' + (rand() % 26));
    copy[i] = randomLetter;
    printf(" BCC1 has been changed\n");
  }
  return copy;
}


unsigned char *messUpBCC2(unsigned char *packet, int sizePacket) {
  unsigned char *copy = (unsigned char *)malloc(sizePacket);
  memcpy(copy, packet, sizePacket);
  int r = (rand() % 100) + 1;
  if (r <= bcc2ErrorPercentage) {
    int i = (rand() % (sizePacket - 5)) + 4;
    unsigned char randomLetter = (unsigned char)('A' + (rand() % 26));
    copy[i] = randomLetter;
    printf(" BCC2 has been changed\n");
  }
  return copy;
}


int LLOPEN(int fd, int x) {

  if (tcgetattr(fd, &oldtio) == -1) { /* save current port settings */
    perror("tcgetattr");
    exit(-1);
  }

  bzero(&newtio, sizeof(newtio));
  newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;

  /* set input mode (non-canonical, no echo,...) */
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

    while (!STOP && !flagAlarm) {
      read(fd, &c, 1);
      stateMachineUA(&state, &c);
    }
  } while (flagAlarm && sumAlarms < NUMMAX);
  printf("flag alarm %d\n", flagAlarm);
  printf("Total Alarms: %d\n", sumAlarms);
  if (flagAlarm && sumAlarms == 3) {
    return FALSE;
  } else {
    flagAlarm = FALSE;
    sumAlarms = 0;
    return TRUE;
  }
}


int LLWRITE(int fd, unsigned char *message, int size) {
  unsigned char BCC2;
  unsigned char *BCC2Stuffed = (unsigned char *)malloc(sizeof(unsigned char));
  unsigned char *finalMessage =
      (unsigned char *)malloc((size + 6) * sizeof(unsigned char));
  int sizefinalMessage = size + 6;
  int sizeBCC2 = 1;
  BCC2 = calculoBCC2(message, size);
  BCC2Stuffed = stuffingBCC2(BCC2, &sizeBCC2);
  int rejected = FALSE;

  finalMessage[0] = FLAG;
  finalMessage[1] = A;
  if (frame == 0) {
    finalMessage[2] = C10;
  } else {
    finalMessage[2] = C11;
  }
  finalMessage[3] = (finalMessage[1] ^ finalMessage[2]);

  int i = 0;
  int j = 4;
  for (; i < size; i++) {
    if (message[i] == FLAG) {
      finalMessage =
          (unsigned char *)realloc(finalMessage, ++sizefinalMessage);
      finalMessage[j] = Escape;
      finalMessage[j + 1] = escapeFlag;
      j = j + 2;
    } else {
      if (message[i] == Escape) {
        finalMessage =
            (unsigned char *)realloc(finalMessage, ++sizefinalMessage);
        finalMessage[j] = Escape;
        finalMessage[j + 1] = escapeEscape;
        j = j + 2;
      } else {
        finalMessage[j] = message[i];
        j++;
      }
    }
  }

  if (sizeBCC2 == 1)
    finalMessage[j] = BCC2;
  else {
    finalMessage =
        (unsigned char *)realloc(finalMessage, ++sizefinalMessage);
    finalMessage[j] = BCC2Stuffed[0];
    finalMessage[j + 1] = BCC2Stuffed[1];
    j++;
  }
  finalMessage[j + 1] = FLAG;

  do {

    unsigned char *copy;
    copy = messUpBCC1(finalMessage, sizefinalMessage); // bcc1
    copy = messUpBCC2(copy, sizefinalMessage);         // bcc2
    write(fd, copy, sizefinalMessage);

    flagAlarm = FALSE;
    alarm(TIMEOUT);
    unsigned char C = readControlMessageC(fd);
    if ((C == CRR1 && frame == 0) || (C == CRR0 && frame == 1)) {
      printf("Received RR: %x frame: %d\n", C, frame);
      rejected = FALSE;
      sumAlarms = 0;
      frame ^= 1;
      alarm(0);
    } else {
      if (C == CREJ1 || C == CREJ0) {
        rejected = TRUE;
        printf("Received REJ: %x frame:%d\n", C, frame);
        alarm(0);
      }
    }
  } while ((flagAlarm && sumAlarms < NUMMAX) || rejected);
  if (sumAlarms >= NUMMAX)
    return FALSE;
  else
    return TRUE;
}


void LLCLOSE(int fd) {
  sendControlMessage(fd, DISC);
  printf("DISC Sended\n");
  unsigned char C;
  // espera ler o DISC
  C = readControlMessageC(fd);
  while (C != DISC) {
    C = readControlMessageC(fd);
  }
  printf(" DISC readed \n");
  sendControlMessage(fd, UA_C);
  printf(" Last UA sended\n");
  printf("Writer terminated \n");

  if (tcsetattr(fd, TCSANOW, &oldtio) == -1) {
    perror("tcsetattr");
    exit(-1);
  }
}