#include "definitions.h"

/*
 * Sends control frame, c is the type of control sent
 */
void sendControlMessage(int fd, unsigned char C);

/*
 * Checks if message's BCC2 is correct
 */
int checkBCC2(unsigned char *message, int sizeMessage);

/*
 * Loop that stops when reads a control frame equals to the parameter C
 */
int readControlMessage(int fd, unsigned char C);

/*
 * Read control frame SET and returns UA
 */
void LLOPEN(int fd);

/*
 * Reads information frames and makes destuffing
 */
unsigned char *LLREAD(int fd, int *sizeMessage);

/*
 * Reads DISC, sends DISC and receives UA.
 */
void LLCLOSE(int fd);
