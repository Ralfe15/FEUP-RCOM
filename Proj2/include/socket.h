#pragma once


#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include "utils.h"
#include "socket.h"

#include <string.h>

#define FTP_PASV 227
#define FTP_FILE_RCV 150

int readSocket(int sockfd);
int readPasv(char *resp, int *port);
int readSck(int sockfd, int *extraReturn) ;
int readCMD(int sockfd, char* resp);
int readCMDSocket(int sockfd, char *cmd, char *arg,char* resp);
int download(int sockfd, char* filename);
int connectTCPsocket(char serverAddress[], int port);
int disconnectFromSocket(int sockfd);