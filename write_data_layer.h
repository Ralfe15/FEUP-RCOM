#include "definitions.h"

/*--------------------------Data Link Layer --------------------------*/

/*
 * Verifica se o UA foi recebido (com alarme).
 * Data link layer
 */
void stateMachineUA(int *state, unsigned char *c);

/*
 * Espera por uma trama de supervisão e retorna o seu C.
 * Data link layer
 */
unsigned char readControlMessageC(int fd);

/*
 * Envia uma trama de supervisão, sendo o C recebido como argumento
 * da função a diferença de cada trama enviada.
 * Data link layer
 */
void sendControlMessage(int fd, unsigned char C);

/*
 * Calcula o valor do BCC2 de uma mensagem.
 * Data link Layer
 */
unsigned char calculoBCC2(unsigned char *mensagem, int size);

/*
 * realiza o stuffing do BCC2.
 * Data link layer
 */
unsigned char *stuffingBCC2(unsigned char BCC2, int *sizeBCC2);

/*
 * Geração aleatória de erros no BCC1.
 * Data link layer
 */
unsigned char *messUpBCC1(unsigned char *packet, int sizePacket);

/*
 * Geração aleatória de erros no BCC2.
 * Data link layer
 */
unsigned char *messUpBCC2(unsigned char *packet, int sizePacket);

/*
 *Envia trama de supervisão SET e recebe trama UA.
 *Data link Layer
 */
int LLOPEN(int fd, int x);

/*
 * Realiza stuffing das tramas I e envia-as.
 * Data link layer
 */
int LLWRITE(int fd, unsigned char *mensagem, int size);
/*
 * Envia trama de supervisão DISC, recebe DISC e envia UA.
 * Data link layer
 */
void LLCLOSE(int fd);