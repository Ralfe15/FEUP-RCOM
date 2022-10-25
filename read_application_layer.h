#include "definitions.h"

/*
 * Get file name from START frame.
 */
unsigned char *nameOfFileFromStart(unsigned char *start);

/*
 * Gets file size through START frame.
 */
off_t sizeOfFileFromStart(unsigned char *start);

/*
 * Removes header from I frames.
 */
unsigned char *removeHeader(unsigned char *toRemove, int sizeToRemove,
                            int *sizeRemoved);

/*
 * Checks if received frame is END.
 */
int isEndMessage(unsigned char *start, int sizeStart, unsigned char *end,
                 int sizeEnd);

/*
 * Create file with data in I frame.
 */
void createFile(unsigned char *mensagem, off_t *sizeFile,
                unsigned char *filename);
