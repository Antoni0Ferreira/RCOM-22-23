// Link layer protocol implementation

#include "link_layer.h"
#include "receptor.h"
#include "transmissor.h"

struct termios oldtio;
struct termios newtio;

int n = 0;

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters)
{
    int fd = open(serialPortName, O_RDWR | O_NOCTTY);

    if (fd < 0)
    {
        perror(serialPortName);
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
    
    if(role == LlTx){
    	unsigned char set[] = {FLAG,A,SET,A^SET,FLAG};
    	sendFrame_t(fd,set,5);
    }
    else if(role == LlRx){
    	unsigned char fr_a, fr_c;
    	unsigned char buffer = malloc(5);
    	int retValue = receiveFrame_r(fd, &fr_a, &fr_c, buffer);
    	if(fr_c == SET && retValue == 0){
			unsigned char cmd[5] = {FLAG,A,UA,BCC,FLAG};
			sendFrame_r(fd, cmd, 5);
    	}
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
    
	n = 0;
	
	int accepted = FALSE;
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
		int newSize = byteStuffing(packet,frameLength,result);
		
		int returnValue = sendFrame_t(fd, result, newSize);
		
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
    unsigned char a,c;
    unsigned char *destuffPacket = malloc(1000);
    
    int r = receiveFrame_r(fd, &a, &c,packet); //consoante o valor retornado pode ser necessario enviar reject
    									  // 0 -> tudo certo, 1-> reject, 2 -> ignorar
    byteDestuffing(packet,1000,destuffPacket);
    
    
    if(r == 0){
    	
	    if(c == 0x00 && expected == 0){
	    	unsigned char cmd[5] = {FLAG,A,RR1,BCC,FLAG};
	    	sendFrame(fd, cmd);
	    	expected = 1;
	    }
	    else if(c == 0x40 && expected == 1){
	        unsigned char cmd[5] = {FLAG,A,RR0,BCC,FLAG};
	    	sendFrame(fd, cmd);
	    	expected = 0;
	    }
	    
    }
    else if(r == 1){
    	if(expected == 0){
	    	unsigned char cmd[5] = {FLAG,A,REJ0,BCC,FLAG};
	    	sendFrame(fd, cmd);
	    }
	    else if(expected == 1){
	        unsigned char cmd[5] = {FLAG,A,REJ1,BCC,FLAG};
	    	sendFrame(fd, cmd);
	    }
    }
        
    return 0;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int fd)
{
	if(role == LlTx){
		unsigned char disc[] = {FLAG,A,DISC,BCC,FLAG};
		r = sendFrame_t(fd, disc, 5);
		
		unsigned char ua[] = {FLAG,0x01,UA,BCC,FLAG};
		r = sendFrame_t(fd, ua, 5);

	}
	else if (role == LlRx){
		unsigned char fr_a, fr_c;
    	unsigned char buffer = malloc(5);
    	int retValue = receiveFrame_r(fd, &fr_a, &fr_c, buffer);
    	if(fr_c == DISC && retValue == 0){
			unsigned char disc[5] = {FLAG,0x01,DISC,BCC,FLAG};
			sendFrame_r(fd, disc,5);
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
