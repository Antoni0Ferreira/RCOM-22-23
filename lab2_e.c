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
#define RR0 0x05
#define RR1 0x85
#define REJ0 0x01
#define REJ1 0x81

volatile int STOP = FALSE;

int alarmEnabled = FALSE;
int alarmCount = 0;
int alarmDone = FALSE;

enum STATE {START, FLAG_RCV, A_RCV, C_RCV, BCC_OK, STP};
enum STATE state = START;

// Alarm function handler
void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    alarmCount++;
    alarmDone = TRUE;

    printf("Alarm #%d\n", alarmCount);
}

int byteStuffing(unsigned char *cmd, int size, unsigned char *result){
	int sizeAux = size;
	int j = 1;
	result[0] = FLAG;
	for(int i = 1; i < size-1; i++){

		if(cmd[i] == 0x7e){ //se encontrar flag
			sizeAux++;
			result = realloc(result,sizeAux);
			result[j] = 0x7d;
			j++;
			result[j] = 0x5e;
		}
		else if(cmd[i] == 0x7d){ //se encontrar esc
			sizeAux++;
			result = realloc(result,sizeAux);
			result[j] = 0x7d;
			j++;
			result[j] = 0x5d;
		
		}
		else result[j] = cmd[i];
		j++;
		 
	}
	result[sizeAux-1] = FLAG;
	return sizeAux;
}

int receiveFrame(int fd){
	unsigned char buf, c;
	
	state = START;
    
    while((state != STP) &&(!alarmDone)){
    	read(fd, &buf, 1);
    	switch (state){
    		case START:
				if(buf == FLAG)
					state = FLAG_RCV;
				break;
			case FLAG_RCV:
				if(buf == A){
					state = A_RCV;
				}
				else if(buf == FLAG)
					state = FLAG_RCV;
				else
					state = START;
				break;
			case A_RCV:
				if(buf == FLAG)
					state = FLAG_RCV;
				else {
					state = C_RCV;
					c = buf;
				}
				break;
			case C_RCV:
				if(buf == FLAG){
					state = FLAG_RCV;
				}
				else
					state = BCC_OK;
				break;
			case BCC_OK:
				if(buf == FLAG){
					state = STP;
				}
				else
					state = START;
				break;
    	}
    }
   
    switch (c){
    	case UA:
    		return 0;
    	case RR0: 
    		return 1;
    	case RR1:
    		return 2;
    	case REJ0:
    		return 3;
    	case REJ1:
    		return 4;
    	case DISC:
    		return 5; 
    }
    
    return -1;
}


int sendFrame(int fd, unsigned char *cmd, int size){ //retorno da funcao -> int consoante o byte C que recebeu (UA, RR, REJ, DISC, !sent)
    int sent = FALSE;
    alarmCount = 0;
    alarmEnabled = FALSE;
    state = START;
    
	int r = -1;
	
    while(alarmCount < 3 && !sent){
        if (alarmEnabled == FALSE){
        
            int bytes = write(fd, cmd, size);
            if(cmd[2] == UA) return 5; 
            printf("escrevi %d\n", bytes);

            alarm(3); // Set alarm to be triggered in 3s

            alarmEnabled = TRUE;
            
            
			r = receiveFrame(fd);
			
			if(state == STP){ //se tiver recebido retransmissão
				sent = TRUE;
				alarm(0);
			}
			printf("alarmCount - %d\n", alarmCount);
            
            //printf("%x\n", bufReceive[i]);

        }
    }
    
    if(alarmCount == 3){
    	r = 6; //numero de tentativas de envio excedidas, terminar execução (return na main)
    }
    
	return r;
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

    // Set alarm function handler
    (void)signal(SIGALRM, alarmHandler);

    printf("New termios structure set\n");
    
    unsigned char set[] = {FLAG,A,SET,BCC,FLAG};
    int size = 5;
    //unsigned char *result = malloc(size);
    //int newSize = byteStuffing(cmd,size,result);
	int r = sendFrame(fd, set, size);
	if(r != 0){
		return 0;
	}
	int index = 0;
	int dataLength = 20;
	int frameLength = 8;
	unsigned char data[] = {0x00, 0x01, 0x02, 0x03, 0x04,
						   0x05, 0x06, 0x07, 0x08, 0x09,
						   0x0A, 0x0B, 0x0C, 0x0D, 0x0E,
						   0x0F, 0x10, 0x11, 0x12, 0x13}; 
	unsigned char *smallData = malloc(frameLength);
	int n = 0;
	while(index < dataLength){
		smallData[0] = FLAG;
		smallData[1] = A;
		if(n == 0){
			smallData[2] = 0x00;
		}
		else{
			smallData[2] = 0x40;
		}
		smallData[3] = smallData[2] ^ A;
		unsigned char BCC2 = 0x00;
		for(int i = 4; i < frameLength - 2;i++){
			BCC2 ^= data[index];
			smallData[i] = data[index];
			index++;
		}
		smallData[frameLength - 2] = BCC2;
		smallData[frameLength - 1] = FLAG;
		
		sendFrame(fd, smallData, frameLength);
		
		BCC2 = 0X00;
		n = (n + 1) % 2;
		printf("index - %d\n", index);
	}
	
	unsigned char disc[] = {FLAG,A,DISC,BCC,FLAG};
    r = sendFrame(fd, disc, size);
    
    unsigned char ua[] = {FLAG,A,UA,BCC,FLAG};
    r = sendFrame(fd, ua, size);
    
	if(r == 6 || r == 5){
		return 0;	//sai, disconnect
	}
	
	state = START;

    

    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    close(fd);

    return 0;
}



