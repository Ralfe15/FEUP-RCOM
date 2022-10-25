#include "read_data_layer.h"
/*--------------------------Data Link Layer --------------------------*/

struct termios oldtio, newtio;
int esperado = 0;

void sendControlMessage(int fd, unsigned char C)
{
    unsigned char message[5];
    message[0] = FLAG;
    message[1] = A;
    message[2] = C;
    message[3] = message[1] ^ message[2];
    message[4] = FLAG;
    write(fd, message, 5);
}

int checkBCC2(unsigned char *message, int sizeMessage)
{
    int i = 1;
    unsigned char BCC2 = message[0];
    for (; i < sizeMessage - 1; i++)
    {
        BCC2 ^= message[i];
    }
    if (BCC2 == message[sizeMessage - 1])
    {
        return TRUE;
    }
    else
        return FALSE;
}

int readControlMessage(int fd, unsigned char C)
{
    int state = 0;
    unsigned char c;

    while (state != 5)
    {
        read(fd, &c, 1);
        switch (state)
        {
        // recebe flag
        case 0:
            if (c == FLAG)
                state = 1;
            break;
        // recebe A
        case 1:
            if (c == A)
                state = 2;
            else
            {
                if (c == FLAG)
                    state = 1;
                else
                    state = 0;
            }
            break;
        // recebe C
        case 2:
            if (c == C)
                state = 3;
            else
            {
                if (c == FLAG)
                    state = 1;
                else
                    state = 0;
            }
            break;
        // recebe BCC
        case 3:
            if (c == (A ^ C))
                state = 4;
            else
                state = 0;
            break;
        // recebe FLAG final
        case 4:
            if (c == FLAG)
            {
                state = 5;
            }
            else
                state = 0;
            break;
        }
    }
    return TRUE;
}

void LLOPEN(int fd)
{

    if (tcgetattr(fd, &oldtio) == -1)
    { /* save current port settings */
        perror("tcgetattr");
        exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME] = 1; /* inter-character timer unused */
    newtio.c_cc[VMIN] = 0;  /* blocking read until 5 chars received */

    /*
      VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
      leitura do(s) próximo(s) caracter(es)
    */

    tcflush(fd, TCIOFLUSH);

    printf("New termios structure set\n");

    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    if (readControlMessage(fd, SET_C))
    {
        printf("SET received\n");
        sendControlMessage(fd, UA_C);
        printf("UA sent\n");
    }
}

unsigned char *LLREAD(int fd, int *sizeMessage)
{
    unsigned char *message = (unsigned char *)malloc(0);
    *sizeMessage = 0;
    unsigned char c_read;
    int trama = 0;
    int mandarDados = FALSE;
    unsigned char c;
    int state = 0;

    while (state != 6)
    {
        read(fd, &c, 1);
        // printf("%x\n",c);
        switch (state)
        {
        // recebe flag
        case 0:
            if (c == FLAG)
                state = 1;
            break;
        // recebe A
        case 1:
            // printf("1state\n");
            if (c == A)
                state = 2;
            else
            {
                if (c == FLAG)
                    state = 1;
                else
                    state = 0;
            }
            break;
        // recebe C
        case 2:
            // printf("2state\n");
            if (c == C10)
            {
                trama = 0;
                c_read = c;
                state = 3;
            }
            else if (c == C11)
            {
                trama = 1;
                c_read = c;
                state = 3;
            }
            else
            {
                if (c == FLAG)
                    state = 1;
                else
                    state = 0;
            }
            break;
        // recebe BCC
        case 3:
            // printf("3state\n");
            if (c == (A ^ c_read))
                state = 4;
            else
                state = 0;
            break;
        // recebe FLAG final
        case 4:
            // printf("4state\n");
            if (c == FLAG)
            {
                if (checkBCC2(message, *sizeMessage))
                {
                    if (trama == 0)
                        sendControlMessage(fd, RR_C1);
                    else
                        sendControlMessage(fd, RR_C0);

                    state = 6;
                    mandarDados = TRUE;
                    printf("Enviou RR, T: %d\n", trama);
                }
                else
                {
                    if (trama == 0)
                        sendControlMessage(fd, REJ_C1);
                    else
                        sendControlMessage(fd, REJ_C0);
                    state = 6;
                    mandarDados = FALSE;
                    printf("Enviou REJ, T: %d\n", trama);
                }
            }
            else if (c == Escape)
            {
                state = 5;
            }
            else
            {
                message = (unsigned char *)realloc(message, ++(*sizeMessage));
                message[*sizeMessage - 1] = c;
            }
            break;
        case 5:
            // printf("5state\n");
            if (c == escapeFlag)
            {
                message = (unsigned char *)realloc(message, ++(*sizeMessage));
                message[*sizeMessage - 1] = FLAG;
            }
            else
            {
                if (c == escapeEscape)
                {
                    message = (unsigned char *)realloc(message, ++(*sizeMessage));
                    message[*sizeMessage - 1] = Escape;
                }
                else
                {
                    perror("Non valid character after escape character");
                    exit(-1);
                }
            }
            state = 4;
            break;
        }
    }
    printf("Message size: %d\n", *sizeMessage);
    // message tem BCC2 no fim
    message = (unsigned char *)realloc(message, *sizeMessage - 1);

    *sizeMessage = *sizeMessage - 1;
    if (!mandarDados)
        *sizeMessage = 0;
    else
    {
        if (trama == esperado)
        {
            esperado ^= 1;
        }
        else
            *sizeMessage = 0;
    }

    return message;
}

/*
 * Reads DISC, sends DISC and receives UA.
 */
void LLCLOSE(int fd)
{
    readControlMessage(fd, DISC_C);
    printf("Recebeu DISC\n");
    sendControlMessage(fd, DISC_C);
    printf("Mandou DISC\n");
    readControlMessage(fd, UA_C);
    printf("Recebeu UA\n");
    printf("Receiver terminated\n");

    tcsetattr(fd, TCSANOW, &oldtio);
}
