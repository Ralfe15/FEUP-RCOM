#include "definitions.h"
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

void prepareSet();
void prepareUA();
void sendFrame();
void receiveFrame(int fd);


/*--------------------------Data Link Layer --------------------------*/

/*
 *Envia trama de supervisão SET e recebe trama UA.
 *Data link Layer
 */
int LLOPEN(int fd, int x);

/*
 * Realiza stuffing das tramas I e envia-as.
 * Data link layer
 */
int LLWRITE(int fd, const unsigned char *buffer, int buf_size) ;
/*
 * Envia trama de supervisão DISC, recebe DISC e envia UA.
 * Data link layer
 */
int LLCLOSE(int fd);

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

/*--------------------------Application Link Layer --------------------------*/

/*
 * Base da camada de aplicação pois é esta que controla todo o processo
 * que ocorre nesta camada e que faz as chamadas às funções da camada
 * de ligação.
 * Application layer
 */
int main(int argc, char **argv);

/*
 * Cria os pacotes de controlo START e END.
 * Application layer
 */
unsigned char *controlPackageI(unsigned char state, off_t sizeFile,
                               unsigned char *fileName, int sizeOfFilename,
                               int *sizeControlPackageI);

/*
 * Abre um ficheiro e le o seu conteúdo.
 * Application layer
 */
unsigned char *openReadFile(unsigned char *fileName, off_t *sizeFile);

/*
 * Acrescenta o cabeçalho do nível de aplicação às tramas.
 * Application layer
 */
unsigned char *headerAL(unsigned char *mensagem, off_t sizeFile,
                        int *sizePacket);

/*
 * Divide uma mensagem proveniente do ficheiro em packets.
 * Application layer
 */
unsigned char *splitMessage(unsigned char *mensagem, off_t *indice,
                            int *sizePacket, off_t sizeFile);