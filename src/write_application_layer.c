#include "write_application_layer.h"
#include "write_data_layer.c"

extern int sumAlarms;
extern int flagAlarm;
unsigned char nMsgs = 0;
int nFrames = 0;
extern struct termios oldtio, newtio;

void alarmHandler() {
  printf("Alarm=%d\n", sumAlarms + 1);
  flagAlarm = TRUE;
  sumAlarms++;
}
unsigned char *controlpacketI(unsigned char state, off_t fileSize_bytes,
                               unsigned char *fileName, int sizeOfFileName,
                               int *sizeControlpacketI) {
  *sizeControlpacketI = 9 * sizeof(unsigned char) + sizeOfFileName;
  unsigned char *packet = (unsigned char *)malloc(*sizeControlpacketI);

  if (state == C2Start)
    packet[0] = C2Start;
  else
    packet[0] = C2End;
  packet[1] = T1;
  packet[2] = L1;
  packet[3] = (fileSize_bytes >> 24) & 0xFF;
  packet[4] = (fileSize_bytes >> 16) & 0xFF;
  packet[5] = (fileSize_bytes >> 8) & 0xFF;
  packet[6] = fileSize_bytes & 0xFF;
  packet[7] = T2;
  packet[8] = sizeOfFileName;
  int k = 0;
  while (k < sizeOfFileName) {
    packet[9 + k] = fileName[k];
    k++;
  }
  return packet;
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

unsigned char *headerAL(unsigned char *message, off_t fileSize_bytes,
                        int *sizePacket) {
  unsigned char *messageFinal = (unsigned char *)malloc(fileSize_bytes + 4);
  messageFinal[0] = headerC;
  messageFinal[1] = nMsgs % 255;
  messageFinal[2] = (int)fileSize_bytes / 256;
  messageFinal[3] = (int)fileSize_bytes % 256;
  memcpy(messageFinal + 4, message, *sizePacket);
  *sizePacket += 4;
  nMsgs++;
  nFrames++;
  return messageFinal;
}

unsigned char *splitMessage(unsigned char *message, off_t *index,
                            int *sizePacket, off_t fileSize_bytes) {
  unsigned char *packet;
  int i = 0;
  off_t j = *index;
  if (*index + *sizePacket > fileSize_bytes) {
    *sizePacket = fileSize_bytes - *index;
  }
  packet = (unsigned char *)malloc(*sizePacket);
  while ( i < *sizePacket) {
    packet[i] = message[j];
    i++;j++;
  }
  *index = j;
  return packet;
}

int main(int argc, char **argv) {

  int fd;
  off_t fileSize_bytes; // file size
  off_t index = 0;
  int sizeControlpacketI = 0;

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

  if (tcgetattr(fd, &oldtio) == -1) { /* save current port settings */
    perror("tcgetattr");
    exit(-1);
  }
  (void)signal(SIGALRM, alarmHandler);

  unsigned char *message =
      openReadFile((unsigned char *)argv[2], &fileSize_bytes);

  struct timespec requestStart, requestEnd;
  clock_gettime(0, &requestStart);

  if (!LLOPEN(fd, 0)) {
    return -1;
  }

  int sizeOfFileName = strlen(argv[2]);
  unsigned char *fileName = (unsigned char *)malloc(sizeOfFileName);
  fileName = (unsigned char *)argv[2];
  unsigned char *start = controlpacketI(C2Start, fileSize_bytes, fileName,
                                         sizeOfFileName, &sizeControlpacketI);

  LLWRITE(fd, start, sizeControlpacketI);
  printf("Sended Start frame\n");

  int sizePacket = sizePacketConst;
  srand(time(NULL));
  for(;sizePacket == sizePacketConst && index < fileSize_bytes;)
 {
    unsigned char *packet =
        splitMessage(message, &index, &sizePacket, fileSize_bytes);
    printf("SENDED packet n: %d\n", nFrames);
    int headerSize = sizePacket;
    unsigned char *messageHeader =
        headerAL(packet, fileSize_bytes, &headerSize);
    if (!LLWRITE(fd, messageHeader, headerSize)) {
      printf("WARNING: Limit os alarms has been trigged.\n");
      return -1;
    }
  }

  unsigned char *end = controlpacketI(C2End, fileSize_bytes, fileName,
                                       sizeOfFileName, &sizeControlpacketI);
  LLWRITE(fd, end, sizeControlpacketI);
  printf("Frame end sended\n");

  LLCLOSE(fd);
  clock_gettime(0, &requestEnd);

  double accum = (requestEnd.tv_sec - requestStart.tv_sec) +
                 (requestEnd.tv_nsec - requestStart.tv_nsec) / 1E9;

  printf("Seconds passed: %f\n", accum);

  sleep(1);

  close(fd);
  return 0;
}