#include "definitions.h"

/*//////////////////
Data Link Layer 
//////////////////*/

/*
 * Verify UA state with alarm
 */
void stateMachineUA(int *state, unsigned char *c);

/*
 * Wait for a frame and returns its C.
 */
unsigned char readControlMessageC(int fd);

/*
* Sends a supervisionFrame.
*  C => difference between each sended frame
 */
void sendControlMessage(int fd, unsigned char C);

/*
 * BCC2 Calculation
 */
unsigned char calculoBCC2(unsigned char *mensagem, int size);

/*
 * BCC2 Stuffing
 */
unsigned char *stuffingBCC2(unsigned char BCC2, int *sizeBCC2);

/*
 * BCC1 erros random generator.
 */
unsigned char *messUpBCC1(unsigned char *packet, int sizePacket);

/*
 * BCC2 erros random generator.
 */
unsigned char *messUpBCC2(unsigned char *packet, int sizePacket);

/*
* Sends Supervision frame and receives frame UA
 */
int LLOPEN(int fd, int x);

/*
 * Stuffing realization and delivery.
 */
int LLWRITE(int fd, unsigned char *mensagem, int size);
/*
 *Send supervision frame and receives DISC and sends UA
 */
void LLCLOSE(int fd);