#include "definitions.h"
/*--------------------------Application Link Layer --------------------------*/

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

/*
 * Base da camada de aplicação pois é esta que controla todo o processo
 * que ocorre nesta camada e que faz as chamadas às funções da camada
 * de ligação.
 * Application layer
 */
int main(int argc, char **argv);