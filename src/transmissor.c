#include "transmissor.h"


volatile int STOP = FALSE;

int alarmEnabled = FALSE;
int alarmCount = 0;
int alarmDone = FALSE;


enum STATE state = START;

// Alarm function handler
void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    alarmCount++;
    alarmDone = TRUE;
	state = START;
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

int receiveFrame_t(int fd){
	unsigned char buf, c;
	
	state = START;
    
    while((state != STP) && (!alarmDone)){
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


int sendFrame_t(int fd, unsigned char *cmd, int size){ //retorno da funcao -> int consoante o byte C que recebeu (UA, RR, REJ, DISC, !sent)
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
            
            
			r = receiveFrame_t(fd);
			
			if(state == STP){ //se tiver recebido retransmissão
				sent = TRUE;
				alarm(0);
			}
			printf("alarmCount - %d\n", alarmCount);
           

        }
    }
    
    if(alarmCount == 3){
    	r = 6; //numero de tentativas de envio excedidas, terminar execução (return na main)
    }
    
	return r;
}
