// Write to serial port in non-canonical mode
//
// Modified by: Eduardo Nuno Almeida [enalmeida@fe.up.pt]

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>

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

volatile int STOP = FALSE;

int alarmEnabled = FALSE;
int alarmCount = 0;
int alarmDone = FALSE;

// Alarm function handler
void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    alarmCount++;
    alarmDone = TRUE;

    printf("Alarm #%d\n", alarmCount);
}

//int sendFrame(unsigned char *buf, int size)
int sendFrame(int fd){
    int sent = FALSE;
    unsigned char cmd[] = {FLAG,A,SET,BCC,FLAG};

    while(alarmCount < 3 && !sent){
        if (alarmEnabled == FALSE)
        {
            int bytes = write(fd, cmd, 5);
            printf("escrevi %d\n", bytes);

            alarm(3); // Set alarm to be triggered in 3s

            alarmEnabled = TRUE;

            unsigned char bufReceive[5] = {0};

            while(!alarmDone && read(fd, bufReceive , 1) == 0){}

            if(alarmDone){
                alarmDone = FALSE;
                continue;
            }

            printf("%x\n", bufReceive[0]);

            for(int i = 1; i < 5; i++){
                read(fd, bufReceive + i, 1);
                printf("%x\n", bufReceive[i]);

            }
            if(bufReceive[2] == 0x07){
                alarm(0);
                sent = TRUE;
            } 
        }

    }
    
}



int main(int argc, char *argv[])
{
    // Program usage: Uses either COM1 or COM2
    const char *serialPortName = argv[1];

    if (argc < 2)
    {
        printf("Incorrect program usage\n"
               "Usage: %s <SerialPort>\n"
               "Example: %s /dev/ttyS1\n",
               argv[0],
               argv[0]);
        exit(1);
    }

    // Open serial port device for reading and writing, and not as controlling tty
    // because we don't want to get killed if linenoise sends CTRL-C.
    int fd = open(serialPortName, O_RDWR | O_NOCTTY);

    if (fd < 0)
    {
        perror(serialPortName);
        exit(-1);
    }

    struct termios oldtio;
    struct termios newtio;

    // Save current port settings
    if (tcgetattr(fd, &oldtio) == -1)
    {
        perror("tcgetattr");
        exit(-1);
    }

    // Clear struct for new port settings
    memset(&newtio, 0, sizeof(newtio));

    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    // Set input mode (non-canonical, no echo,...)
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 0; // Inter-character timer unused
    newtio.c_cc[VMIN] = 0;  // Blocking read until 5 chars received

    // VTIME e VMIN should be changed in order to protect with a
    // timeout the reception of the following character(s)

    // Now clean the line and activate the settings for the port
    // tcflush() discards data written to the object referred to
    // by fd but not transmitted, or data received but not read,
    // depending on the value of queue_selector:
    //   TCIFLUSH - flushes data received but not read.
    tcflush(fd, TCIOFLUSH);

    // Set new port settings
    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    // Set alarm function handler
    (void)signal(SIGALRM, alarmHandler);

    printf("New termios structure set\n");
    unsigned char cmd[] = {FLAG,A,SET,BCC,FLAG};
    //unsigned char cmd2[] = {FLAG,A,DISC,BCC,FLAG};
    

    // int bytes2 = write(fd, cmd2, 5);
    // for(int i = 0; i < 5; i++){
    //     printf("%x\n",cmd2[i]);
    // }
	sendFrame(fd);

    

    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    close(fd);

    return 0;
}

