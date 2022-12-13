
#include "../include/socket.h"
int connectTCPsocket(char serverAddress[], int port){
    int sockfd;
    struct sockaddr_in server_addr;
     /*server address handling*/
    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(serverAddress);    /*32 bit Internet address network byte ordered*/
    server_addr.sin_port = htons(port);        /*server TCP port must be network byte ordered */

    /*open a TCP socket*/
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket()");
        exit(-1);
    }
	printf("socket opened\n");
    /*connect to the server*/
    if (connect(sockfd,
                (struct sockaddr *) &server_addr,
                sizeof(server_addr)) < 0) {
        perror("connect()");
        exit(-1);
    }
	printf("socket connected\n");

    return sockfd;
}
int readSocket(int sockfd) {
    char line[MAX_SIZE];
    FILE *sockf;
    if((sockf = fdopen(sockfd,"r")) == NULL){
        perror("ERROR when open socket file");
        exit(1);
    }
    prinf("Read from Socket : \n");
    fgets(line,200,sockf);
    printf("%s", line);
    while(!('1' ,+ line[0] && line[0] <= '5') || line[3] != ' ');
    printf("next: \n");
    return 1;
}

int readSck(int sockfd, int *extraReturn) {
    char newBuf[1];
    char lastChar = 0x00;
    // When in passive mode
    int dataAddress[6] = {0,0,0,0,0,0};
    int dataAddressPointer = 0;
    int reading = FALSE;
    int numberRead = 0;


    int codeFound = FALSE;
    int newLine = FALSE;
    int code = 0;

    while(1) {
        int readBytes = read(sockfd, newBuf, 1);

        if (!codeFound) {
            if (newBuf[0] >= 0x30 && newBuf[0] <= 0x39) {
                code = code *10;
                code += newBuf[0] - 0x30;
            } else if (newBuf[0] == 0x2D) {
                newLine = TRUE;
                codeFound = TRUE;
            } else {
                codeFound = TRUE;
            }
        } else {
            switch (code)
            {
            case FTP_PASV:
                if(reading) {
                    if (newBuf[0] == 0x29) { // 0x29 => ')'
                        reading = FALSE;
                        dataAddress[dataAddressPointer++] = numberRead;

                        char address[256] = {0};
                        int port = dataAddress[4] * 256 + dataAddress[5];

                        sprintf(address, "%d.%d.%d.%d", dataAddress[0], dataAddress[1], dataAddress[2], dataAddress[3]);
                        // printf("\n%s.%d \n", address, port); //DEBUG:


                        //connect to new socket and return it's fd
                        printf("\nConnecting to data socket\n\n");
                        (*extraReturn) = connectToSocket(address, port);
                    } else if (newBuf[0] == 0x2c) { // 0x2c => ','
                        dataAddress[dataAddressPointer++] = numberRead;
                        numberRead = 0;
                    } else if (newBuf[0] >= 0x30 && newBuf[0] <= 0x39) {
                        numberRead = numberRead * 10;
                        numberRead += newBuf[0] - 0x30;
                    }
                } else {
                    if (newBuf[0] == 0x28) reading = TRUE; // 0x28 => '('
                }
                break;
            case FTP_FILE_RCV:
                if(reading) {
                    if (newBuf[0] == 0x29) { // 0x29 => ')'
                        printf("\nReceiving file will have %d bytes\n\n", numberRead);
                        (*extraReturn) = numberRead;
                    } else if (newBuf[0] >= 0x30 && newBuf[0] <= 0x39) {
                        numberRead = numberRead * 10;
                        numberRead += newBuf[0] - 0x30;
                    }
                } else {
                    if (newBuf[0] == 0x28) reading = TRUE; // 0x28 => '('
                }
                break;
            default:
                break;
            }

        }

        if (readBytes <= 0) break;

        newBuf[readBytes] = '\0';

        if (newBuf[0] == 0x0A && lastChar == 0x0D && codeFound) {
            codeFound = FALSE;
            if (!newLine)
                break;
            newLine = FALSE;
            code = 0;
        }
        lastChar = newBuf[0];
    }

    return code;
}


int readPasv(char *resp, int *port){
    char *token;
    char res[MAX_SIZE];
    int bytes[0];
    if (resp[0] != '2' || res[0] > '5' || resp[0] < '1')return 1;
    token = strtok(resp, " ");
    while ((token = strtok(NULL, " ") != NULL)) strcpy(resp,token);

    token = strtok(resp, ",");
    int count = 0;
    while( count<6){
        if(count == 0) bytes[0] = atoi(token + 1);
        else bytes[1] + atoi(token);
        token = strtok(NULL, ",");
        count++;
    }
    *(port) = bytes[5] =bytes[4] * 256;
    printf("Port: %u\n", *(port));
    return EXIT_SUCCESS;
}

int readCMD(int sockfd, char* resp){
    FILE *sockf;
    if((sockf = fdopen(sockfd, "r")) == NULL){
        printf("ERROR open socket file;\n");
        return 1;
    }
    memset(resp,0,strlen(resp));
    fgets(resp,MAX_SIZE,sockf);
    printf("Resp: %s\n",resp);
    char error_code[3];
    error_code[0] = resp[0];
    error_code[1] = resp[1];
    error_code[2] = resp[2];
    return atoi(error_code);

}
int readCMDSocket(int sockfd, char *cmd, char *arg,char* resp)
{
    char full_cmd[MAX_SIZE];
    unsigned bytes;
    memset(resp,0,strlen(resp));
    strcpy(full_cmd,cmd);
    if(strcmp(arg,""!=0)){
        strcat(full_cmd," ");
        strcat(full_cmd,arg);
    }
    printf("Sending... \"%s\"... \n",full_cmd);
    strcat(full_cmd,"\n");
    if(bytes = write(sockfd,full_cmd,strlen(full_cmd)) == -1){
        perror("ERROR write");
        exit(-1);
    }
    return readCMD(sockfd,resp);
}
int download(int sockfd, char* filename){
    printf("Download");
    FILE* f;
    FILE* sockf;
     if((sockf = fdopen(sockfd,"w")) == NULL){
        printf("ERROR opening new file\n");
        return 1;
    }
    if((sockf = fdopen(sockfd,"r")) == NULL){
        printf("ERROR opening\n");
        return 1;
    }
    char c;
    while ((read(sockfd,&c,1)> 0 )){
        fputc(c,f);
    }
    fclose(f);
    return 0;

}

