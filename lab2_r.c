// Read from serial port in non-canonical mode
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

volatile int STOP = FALSE;
enum STATE {START, FLAG_RCV, A_RCV, C_RCV, BCC_OK, DATA_RCVG, BCC2_OK, STP};

enum STATE state = START;

int destuffing(unsigned char *cmd, int size, unsigned char *result){
	
	int foundEsc = FALSE;
	int j = 0;
	for(int i = 0; i < size; i++){
		if(foundEsc){
			if(cmd[i] == 0x5E){
				result[j] = 0x7E;
			}
			else if(cmd[i] == 0x5D){
				result[j] = 0x7D;
			}
			j++;
			foundEsc = FALSE;
		}
		else{
			if(cmd[i] == 0x7D){
				foundEsc = TRUE;
			}
			else{
				result[j] = cmd[i];
				j++;
			}
		}
	}
	return j; 
	
}

int sendFrame(int fd, unsigned char *cmd){

	int bytes = write(fd, cmd, 5);
	return 0;
}


int receiveFrame(int fd, unsigned char *fr_a, unsigned char *fr_c, unsigned char *buffer){
    
    printf("frame: ");
    while(state != STP){
        unsigned char buf; 
        unsigned char bcc2;

        int bytes = read(fd, &buf, 1);
        if(bytes == 0) {
        	continue;
        }
		printf("%x\n", buf);

        int index = 1;
        
        switch (state) {
            case START:
            	
                if(buf == FLAG)
                    state = FLAG_RCV;
                break;
            case FLAG_RCV:
            	
                if(buf == A){
                    state = A_RCV;
                    (*fr_a) = buf;
                }
                else if(buf == FLAG)
                    state = FLAG_RCV;
                else
                    state = START;
                break;
            case A_RCV:
                if( TRUE){
                    state = C_RCV;
                    (*fr_c) = buf;
                }
                else if(buf == FLAG)
                    state = FLAG_RCV;
                else
                    state = START;
                break;
            case C_RCV:
            	
                if(buf == FLAG){
                    state = FLAG_RCV;
                }
                else
                    state = BCC_OK;
                break;
            case BCC_OK:
            	
            	if((*fr_c) == 0x40 || (*fr_c) == 0x00){
            		state = DATA_RCVG;
            		buffer[0] = buf;
            	}
            	else{
            		if(buf == FLAG)
                    	state = STP;
                	else
                    	state = START;
                
            	}
            	break;
            case DATA_RCVG:
            	if(buf == FLAG /*e se ultimo byte lido não tiver sido esc*/)
            	{	state = STP;
            		bcc2 = buffer[index-1];
            	}
            	//if (buf == ESC) pôr boleano a true (lastWasEsc)
            	else{
            		buffer[index] = buf;
            		index += 1;
            		
            	}
            	
            	
                
            
        }
        
    }
    return 0;
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

    // Open serial port device for reading and writing and not as controlling tty
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
    newtio.c_cc[VTIME] = 30; // Inter-character timer unused
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

    printf("New termios structure set\n");
    int disconnect = FALSE;
    
    while(!disconnect){

        unsigned char a,c;
        unsigned char *buf = malloc(1000);
        unsigned char *destuffBuf = malloc(1000);
        
        receiveFrame(fd, &a, &c,buf);
        destuffing(buf,1000,destuffBuf);
        state = START;
        
        if(c == SET){
        	unsigned char cmd[5] = {FLAG,A,UA,BCC,FLAG};
        	sendFrame(fd, cmd);
        }
        else if(c == DISC){
        	unsigned char cmd[5] = {FLAG,A,DISC,BCC,FLAG};
        	sendFrame(fd, cmd);
        }
        else if(c == UA){
        	disconnect = TRUE;
        }
        else if(c == 0x00){
        	unsigned char cmd[5] = {FLAG,A,RR0,BCC,FLAG};
        	sendFrame(fd, cmd);
        }
        else if(c == 0x40){
            unsigned char cmd[5] = {FLAG,A,RR1,BCC,FLAG};
        	sendFrame(fd, cmd);
        }
    
    }

    // The while() cycle should be changed in order to respect the specifications
    // of the protocol indicated in the Lab guide

    // Restore the old port settings
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    close(fd);

    return 0;
}

