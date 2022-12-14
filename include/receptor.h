#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

// Baudrate settings are defined in <asm/termbits.h>, which is
// included by <termios.h>
#define BAUDRATE B38400
#define _POSIX_SOURCE 1 // POSIX compliant source

#define FALSE 0
#define TRUE 1

#define BUF_SIZE 256

#define FLAG 0x7E
#define A 0x03
#define SET 0x03
#define UA 0x07
#define BCC (A ^ SET)
#define DISC 0x0B
#define RR0 0x05
#define RR1 0x85
#define REJ0 0x01
#define REJ1 0x81

enum STATE {START, FLAG_RCV, A_RCV, C_RCV, BCC_OK, DATA_RCVG, BCC2_OK, STP};

int byteDestuffing(unsigned char *cmd, int size, unsigned char *result);

int sendFrame_r(int fd, unsigned char *cmd);

int receiveFrame_r(int fd, unsigned char *fr_a, unsigned char *fr_c, unsigned char *buffer);




