#include "receptor.h"

volatile int STOP = FALSE;

enum STATE state = START;

int byteDestuffing(unsigned char *cmd, int size, unsigned char *result){
	
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

int sendFrame_r(int fd, unsigned char *cmd){

	int bytes = write(fd, cmd, 5);
	return 0;
}


int receiveFrame_r(int fd, unsigned char *fr_a, unsigned char *fr_c, unsigned char *buffer){
    state = START;
    
    int correctBcc1 = TRUE, correctBcc2 = TRUE;
    unsigned char bcc2,bcc2Verify = 0x00;
    int index = 1;
    unsigned char a = 0x00, c = 0x00;
    while(state != STP){
        unsigned char buf; 
        

        int bytes = read(fd, &buf, 1);
        if(bytes == 0) {
        	continue;
        }
		printf("%x\n", buf);

        
        
        switch (state) {
            case START:
            	
                if(buf == FLAG)
                    state = FLAG_RCV;
                break;
            case FLAG_RCV:
            	
                if(buf == A){
                    state = A_RCV;
                    (*fr_a) = buf;
                    a = buf;
                }
                else if(buf == FLAG)
                    state = FLAG_RCV;
                else
                    state = START;
                break;
            case A_RCV:
            	if(buf == FLAG)
                    state = FLAG_RCV;
                else{
                    state = C_RCV;
                    (*fr_c) = buf;
                    c = buf;
                }
                
                break;
            case C_RCV:
            	
                if(buf == FLAG){
                    state = FLAG_RCV;
                }
                else{
                	state = BCC_OK;
                	unsigned char xor = a^c;
                	unsigned char compare = buf;
                	if(memcmp(&xor, &compare, 1)){
                		correctBcc1 = FALSE;
                	}
                }
                    
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
            	else{
            		buffer[index] = buf;
            		index += 1;
            	}            
        }
    }
    for(int i = 0; i < index -1 ; i++){
    	bcc2Verify ^= buffer[i];
    }
    if(bcc2Verify != bcc2){
    	correctBcc2 = FALSE;
    }

    if(correctBcc1 && !correctBcc2){ //enviar reject

    	return 1;
    }
    if(!correctBcc1){

    	return 2;
    }
    return 0;
}
