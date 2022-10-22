#include "receptor.h"

volatile int STOP_r = FALSE;

enum STATE state_r = START;

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

	write(fd, cmd, 5);
	return 0;
}


int receiveFrame_r(int fd, unsigned char *fr_a, unsigned char *fr_c, unsigned char *buffer){
    state_r = START;
    
    int correctBcc1 = TRUE;
    int index = 1;
    unsigned char a = 0x00, c = 0x00;
    while(state_r != STP){
        unsigned char buf; 
        

        int bytes = read(fd, &buf, 1);
        if(bytes == 0) {
        	continue;
        }
		//printf("%x ", buf);

        
        
        switch (state_r) {
            case START:
            	
                if(buf == FLAG)
                    state_r = FLAG_RCV;
                break;
            case FLAG_RCV:
            	
                if(buf == A || buf == 0x01){
                    state_r = A_RCV;
                    (*fr_a) = buf;
                    a = buf;
                }
                else if(buf == FLAG)
                    state_r = FLAG_RCV;
                else
                    state_r = START;
                break;
            case A_RCV:
            	if(buf == FLAG)
                    state_r = FLAG_RCV;
                else{
                    state_r = C_RCV;
                    (*fr_c) = buf;
                    c = buf;
                }
                
                break;
            case C_RCV:
            	
                if(buf == FLAG){
                    state_r = FLAG_RCV;
                }
                else{
                	state_r = BCC_OK;
                	unsigned char xor = a^c;
                	unsigned char compare = buf;
                	if(memcmp(&xor, &compare, 1)){
                		correctBcc1 = FALSE;
                	}
                }
                    
                break;
            case BCC_OK:
            	
            	if((*fr_c) == 0x40 || (*fr_c) == 0x00){
            		state_r = DATA_RCVG;
            		buffer[0] = buf;
            	}
            	else{
            		if(buf == FLAG)
                    	state_r = STP;
                	else
                    	state_r = START;
                
            	}
            	break;
            case DATA_RCVG:
            	if(buf == FLAG /*e se ultimo byte lido não tiver sido esc*/)
            	{	state_r = STP;
            		//bcc2 = buffer[index-1];
            	}
            	else{
            		buffer[index] = buf;
            		index += 1;
            	}            
        }
    }
    /*
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
    }*/
    printf("\n");
    return correctBcc1;
}
