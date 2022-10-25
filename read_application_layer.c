/*--------------------------Application Link Layer --------------------------*/
#include "read_data_layer.c"
#include "read_application_layer.h"

unsigned char *nameOfFileFromStart(unsigned char *start)
{

    int L2_ = (int)start[8];
    unsigned char *name = (unsigned char *)malloc(L2_ + 1);

    int i = 0;
    while (i < L2_)
    {
        name[i] = start[9 + i];
        i++;
    }

    name[L2_] = '\0';
    return name;
}

off_t sizeOfFileFromStart(unsigned char *start)
{
    return (start[3] << 24) | (start[4] << 16) | (start[5] << 8) | (start[6]);
}

unsigned char *removeHeader(unsigned char *toRemove, int sizeToRemove, int *sizeRemoved)
{
    int i = 0;
    int j = 4;
    unsigned char *messageRemovedHeader = (unsigned char *)malloc(sizeToRemove - 4);
    while (i < sizeToRemove)
    {
        messageRemovedHeader[i] = toRemove[j];
        i++;
        j++;
    }
    *sizeRemoved = sizeToRemove - 4;
    return messageRemovedHeader;
}

int isEndMessage(unsigned char *start, int sizeStart, unsigned char *end, int sizeEnd)
{
    int s = 1;
    int e = 1;
    if (sizeStart == sizeEnd)
    {
        if (end[0] == C2End)
        {
            while (s < sizeStart)
            {
                if (start[s] != end[e])
                    return FALSE;
                s++;
                e++;
            }
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
    else
    {
        return FALSE;
    }
}

void createFile(unsigned char *mensagem, off_t *sizeFile, unsigned char *filename)
{
    FILE *file = fopen((char *)filename, "wb+");
    fwrite((void *)mensagem, 1, *sizeFile, file);
    printf("%zd\n", *sizeFile);
    printf("New file created\n");
    fclose(file);
}

/*
 * Makes calls do data layer and base of all the process.
 */
int main(int argc, char **argv)
{
    int fd;
    int sizeMessage = 0;
    unsigned char *mensagemPronta;
    int sizeOfStart = 0;
    unsigned char *start;
    off_t sizeOfPenguin = 0;
    unsigned char *penguin;
    off_t index = 0;

    if ((argc < 2) ||
        ((strcmp("/dev/ttyS10", argv[1]) != 0) &&
         (strcmp("/dev/ttyS11", argv[1]) != 0)))
    {
        printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS10\n");
        exit(1);
    }
    /*
      Open serial port device for reading and writing and not as controlling tty
      because we don't want to get killed if linenoise sends CTRL-C.
    */
    fd = open(argv[1], O_RDWR | O_NOCTTY);
    if (fd < 0)
    {
        perror(argv[1]);
        exit(-1);
    }

    LLOPEN(fd);
    start = LLREAD(fd, &sizeOfStart);

    unsigned char *nameOfFile = nameOfFileFromStart(start);
    sizeOfPenguin = sizeOfFileFromStart(start);

    penguin = (unsigned char *)malloc(sizeOfPenguin);

    while (TRUE)
    {
        mensagemPronta = LLREAD(fd, &sizeMessage);
        if (sizeMessage == 0)
            continue;
        if (isEndMessage(start, sizeOfStart, mensagemPronta, sizeMessage))
        {
            printf("End message received\n");
            break;
        }

        int sizeWithoutHeader = 0;

        mensagemPronta = removeHeader(mensagemPronta, sizeMessage, &sizeWithoutHeader);

        memcpy(penguin + index, mensagemPronta, sizeWithoutHeader);
        index += sizeWithoutHeader;
    }

    printf("Mensagem: \n");
    int i = 0;
    for (; i < sizeOfPenguin; i++)
    {
        printf("%x", penguin[i]);
    }

    createFile(penguin, &sizeOfPenguin, nameOfFile);

    LLCLOSE(fd);

    sleep(1);

    close(fd);
    return 0;
}