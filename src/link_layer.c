
// Link layer protocol implementation

#include "../include/link_layer.h"
#include "../include/receptor.h"
#include "../include/transmissor.h"

#include <stdlib.h>

struct termios oldtio;
struct termios newtio;

extern FILE *receptorFptr;
int fd;
int n = 0;
LinkLayerRole role;
int numTries;
int timeout;
int expected = 0;

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters)
{
	printf("serialPort - %s\n", connectionParameters.serialPort);

    fd = open(connectionParameters.serialPort, O_RDWR | O_NOCTTY);
    
    

    if (fd < 0)
    {
        perror(connectionParameters.serialPort);
        exit(-1);
    }

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
    
    timeout = connectionParameters.timeout;
    numTries = connectionParameters.nRetransmissions;
    
    if(connectionParameters.role == LlTx){
    	unsigned char set[] = {FLAG,A,SET,A^SET,FLAG};
    	sendFrame_t(fd,set,5,timeout,numTries);
    	role = LlTx;
    }
    else if(connectionParameters.role == LlRx){
    	unsigned char fr_a, fr_c;
    	unsigned char *buffer = malloc(5);
    	int retValue = receiveFrame_r(fd, &fr_a, &fr_c, buffer);
    	if(fr_c == SET && retValue){
			unsigned char cmd[5] = {FLAG,A,UA,BCC,FLAG};
			sendFrame_r(fd, cmd);
    	}
    	role = LlRx;
    }

    return fd;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{
	int frameLength = bufSize + 6;
	unsigned char *packet = malloc(frameLength);
    

	
	int accepted = FALSE;
	int index = 0;
	while(!accepted){
		
		packet[0] = FLAG;
		packet[1] = A;
		if(n == 0){
			packet[2] = 0x00;
		}
		else{
			packet[2] = 0x40;
		}
		packet[3] = packet[2] ^ A;
		unsigned char BCC2 = 0x00;
		for(int i = 4; i < frameLength - 2;i++){
			BCC2 ^= buf[index];
			packet[i] = buf[index];
			index++;
		}
		
		packet[frameLength - 2] = BCC2;
		packet[frameLength - 1] = FLAG;
		unsigned char *result = malloc(frameLength);
		for(int i = 0; i < frameLength; i++){
			printf("%x ", packet[i]);
		}
		int newSize = byteStuffing(packet,frameLength,result);
		
		printf("\n");
		int returnValue = sendFrame_t(fd, result, newSize, timeout, numTries);
		
		if(returnValue != 3 && returnValue != 4) accepted = TRUE;
		
		n = (n + 1) % 2;

	}

    return 0;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    unsigned char headerA, headerC;
    unsigned char *destuffPacket = malloc(1000);
    
    int bcc1 = receiveFrame_r(fd, &headerA, &headerC,destuffPacket); //consoante o valor retornado pode ser necessario enviar reject
    									  // 1 -> bcc1 correto, 0 bcc1 errado
  	
    byteDestuffing(destuffPacket,1000,packet);
    
    
    
		int r = 0;
		if(packet[0] == 0x01){
		
			int size = packet[2] * 256 + packet[3] + 4;
			
			unsigned char bcc2 = packet[size], bcc2Verify = 0x00;
			
			for(int i = 0; i < size ; i++){
				bcc2Verify ^= packet[i];
				//printf("destuffPacket[%d] - %x, bcc2Verify - %x\n", i, destuffPacket[i], bcc2Verify);
			}
			printf("bcc2Verify - %x, bcc2 - %x\n", bcc2Verify, bcc2);
			if(bcc2Verify == bcc2 && bcc1){
				r = 0;
			}
			else if (bcc1){
				r = 1;
			}
		}
		
		printf("r - %d, c - %x\n", r, headerC);
		if(r == 0){
			
			if(headerC == 0x00 && expected == 0){
				printf("recebi o 0, manda o 1\n");
				unsigned char cmd[5] = {FLAG,A,RR1,BCC,FLAG};
				sendFrame_r(fd, cmd);
				expected = 1;
			}
			else if(headerC == 0x40 && expected == 1){
				printf("recebi o 1, manda o 0\n");
			    unsigned char cmd[5] = {FLAG,A,RR0,BCC,FLAG};
				sendFrame_r(fd, cmd);
				expected = 0;
			}
			
		}
		else if(r == 1){
			if(expected == 0){
				unsigned char cmd[5] = {FLAG,A,REJ0,BCC,FLAG};
				sendFrame_r(fd, cmd);
			}
			else if(expected == 1){
			    unsigned char cmd[5] = {FLAG,A,REJ1,BCC,FLAG};
				sendFrame_r(fd, cmd);
			}
		}

		if(packet[0] == 3) return 1;
		    
		
	
	return 0;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int fd)
{
	
	if(role == LlTx){
		
		unsigned char disc[] = {FLAG,A,DISC,BCC,FLAG};
		sendFrame_t(fd, disc, 5, timeout, numTries);
		
		unsigned char ua[] = {FLAG,0x01,UA,BCC,FLAG};
		sendFrame_t(fd, ua, 5, timeout, numTries);

	}
	else if (role == LlRx){
		unsigned char fr_a, fr_c;
    	unsigned char *buffer = malloc(5);
    	int retValue = receiveFrame_r(fd, &fr_a, &fr_c, buffer);
    	if(fr_c == DISC && retValue == 0){
			unsigned char disc[5] = {FLAG,0x01,DISC,BCC,FLAG};
			sendFrame_r(fd, disc);
    	}
    	retValue = receiveFrame_r(fd, &fr_a, &fr_c, buffer);
    	
	}
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    close(fd);

    return 1;
}
