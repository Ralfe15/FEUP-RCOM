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
#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1
// Flags for class 2
#define FLAG 0x7E
#define A 0x03
#define C_SET 0x03
#define C_UA 0x07

#define MODEMDEVICE "/dev/ttyS1"
#define NUMMAX 3
#define TIMEOUT 3
#define sizePacketConst 100
#define bcc1ErrorPercentage 0
#define bcc2ErrorPercentage 0

#define SET_BCC (A ^ SET_C)
#define UA_BCC (A ^ UA_C)
#define UA_C 0x07
#define SET_C 0x03
#define C10 0x00
#define C11 0x40
#define C2Start 0x02
#define C2End 0x03
#define CRR0 0x05
#define CRR1 0x85
#define CREJ0 0x01
#define CREJ1 0x81
#define DISC 0x0B
#define headerC 0x01

#define Escape 0x7D
#define escapeFlag 0x5E
#define escapeEscape 0x5D

#define T1 0x00
#define T2 0x01
#define L1 0x04
#define L2 0x0B

#define RR_C0 0x05
#define RR_C1 0x85
#define REJ_C0 0x01
#define REJ_C1 0x81
#define DISC_C 0x0B
#define C2End 0x03
