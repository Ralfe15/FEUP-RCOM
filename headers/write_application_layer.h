#include "definitions.h"
/*////////////////////////////
Application Link Layer 
////////////////////////////*/

/*
 * START/End Control Packet
 */
unsigned char *controlPacketI(unsigned char state, off_t sizeFile,
                               unsigned char *fileName, int sizeOfFilename,
                               int *sizeControlPackageI);

/*
 * Handles File Read.
 */
unsigned char *openReadFile(unsigned char *fileName, off_t *sizeFile);

/*
  * Add Applicatiation Header to frames
 */
unsigned char *headerAL(unsigned char *mensagem, off_t sizeFile,
                        int *sizePacket);

/*
*    Split a message into packets.
 */
unsigned char *splitMessage(unsigned char *mensagem, off_t *indice,
                            int *sizePacket, off_t sizeFile);

/*
 * Application Layer main Function
 */
int main(int argc, char **argv);