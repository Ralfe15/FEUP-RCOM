#include "include/socket.h"
#include "include/util.h"


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
    printf(">>%s", argv[1]);
    char host_name[20] = {0};
    printf("\nhost_name_toString: %d\n", gethost_nameString(argv[1], host_name));
    printf("host_name: %s\n", host_name);
    
    getIP(host_name);
}