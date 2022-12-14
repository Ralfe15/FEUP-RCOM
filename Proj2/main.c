#include "include/socket.h"
#include "include/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

typedef char url_data[MAX_SIZE];

typedef struct URL {
	url_data user; 
	url_data password; 
	url_data host; 
	url_data path; 
	url_data filename;
    url_data input;
    url_data protocol;
	int port; 
} url;

enum parseState {read_FTP, read_USR, read_PASS, read_HOST, read_PATH};
// ftp://[<user>:<password>@]<host>/<url-path>
int parseURL(char **args, url* url){
    char *arg =args[2];

    if(strcmp(args[1],"download") != 0)
    {
        printf("%s invalid command\n",args[1]);
        return EXIT_FAILURE;
    }
    strcpy(url->input,args[1]);
    char *parser = strtok(arg,"/");

    if(!strcmp(strtok(arg,"/"),"ftp:")){
        printf("Invalid Protocol, the correct is ftp\n");
        return EXIT_FAILURE;
    }
    strcpy(url->protocol,parser);
    char *c_host = strtok(NULL, "/");

    char *dir;
    memset(url->path,0,strlen(url->path));
    while((dir = strtok(NULL,"/")) != NULL){
        strcat(url->path,"/");
        strcat(url->path,dir);
        strcpy(url->filename,dir);
    }
    char *credentials = strtok(c_host,"@");
    strcpy(url->host,strtok(NULL, "@"));
    strcpy(url->user,strtok(credentials,":"));
    if(strtok(NULL, ":")) strcpy(url->password,"");
    else strcpy(url->password,strtok(NULL,":"));
    return EXIT_SUCCESS;

}
int getIP(char *host, char *host_ip){
    struct hostent *h;
       if ((h = gethostbyname(host)) == NULL) {
        herror("gethostbyname()");
        exit(-1);
    }
    printf("Host name  : %s\n", h->h_name);
    printf("IP Address : %s\n", inet_ntoa(*((struct in_addr *) h->h_addr)));
    strcpy(host_ip, inet_ntoa(*((struct in_addr *) h->h_addr)));
    return 0;
}

int gethost_name_toString(char hostString[], char host_name[]) {
    int idx = 0;
    int maxSize = strlen(hostString);
    int counter = 0;
    for (int i = 1; i <= maxSize; i++) {
        if (counter && hostString[i] == '/') break;
        if (hostString[i] == '/' && hostString[i-1] == '/') {
            counter = 1;
            continue;
        }
        if (counter) host_name[idx++] = hostString[i];
    }
    host_name[idx] = '\0';
    return EXIT_SUCESS;
}





int main(int argc, char **argv) {
  

	struct URL url;
    char ip[MAX_SIZE];
    char reader[MAX_SIZE];
    int datasocketfd,port;
      if (argc != 3 || parseURL(argv,&url)) {
		fprintf(stderr, "Wrong number of arguments!");
		return 1;
	}
    parseURL(&url,argv[1]);

    printf("Username: %s\n", url.user);
	printf("Password: %s\n", url.password);
	printf("Host: %s\n", url.host);
	printf("Path: %s\n", url.path);
	printf("Filename: %s\n", url.filename);

    struct hostent *h;
    
    h = getIP(url.host,ip);

    printf("ip: %s\n", ip);
    printf("port: %d\n", url.port);

    int socketfd;
    if((socketfd = connectTCPsocket(ip,url.port)) == -1)
    {
		fprintf(stderr, "Unable to connect to socket");
		exit(1);
	}

    readSkt(socketfd, NULL);
    
    int response = 0;

    // Sending username
    if(readCMDSocket(socketfd,"user", url.user,response)!= 331){
        perror("ERROR sending user");
        exit(1);
    }
      if(readCMDSocket(socketfd,"pass", url.password,response)!= 230){
        perror("ERROR sending pass");
        exit(1);
    }  if(readCMDSocket(socketfd,"pasv", url.password,response)!= 227){
        perror("ERROR sending pasv");
        exit(1);
    } 
    readPasv(response,&port);
     if((datasocketfd = connectTCPsocket(ip,port)) == -1)
    {
		fprintf(stderr, "Unable to connect to socket");
		exit(1);
	}
    if(readCMDSocket(socketfd,"retr",url.path,response != 150)){
        perror("ERROR retr");
        exit(1);
    }
    if(download(datasocketfd,url.filename)==1){     
           perror("ERROR download");
        exit(1);}
        else printf("download sucess");

 if(close(socketfd) < 0){     
           perror("ERROR close");
        exit(1);} 
        if(close(datasocketfd) < 0){     
           perror("ERROR close");
        exit(1);} 
       
    //Check what the size of the file is
    
  

    return 0;
}