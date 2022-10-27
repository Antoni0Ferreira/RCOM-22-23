#include "transmissor.h"
#include "receptor.h"


volatile int STOP_t = FALSE;

int alarmEnabled = FALSE;
int alarmCount = 0;


enum STATE state_t = START;

// Alarm function handler
void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    alarmCount++;
    printf("Alarm #%d\n", alarmCount);
}

int byteStuffing(unsigned char *cmd, int size, unsigned char *result){
	int sizeAux = size;
	int j = 1;
	result[0] = FLAG;
	printf("antes do stuffing, size = %d\n",size); 
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
	printf("depois do stuffing, size = %d\n", sizeAux);
	result[sizeAux-1] = FLAG;
	return sizeAux;
}

int receiveFrame_t(int fd){
	unsigned char buf, c;
	
	state_t = START;
    
    while((state_t != STP) && alarmEnabled){
    	int bytes = read(fd, &buf, 1);
    	if(bytes == 0) continue;
    	switch (state_t){
    		case START:
				if(buf == FLAG)
					state_t = FLAG_RCV;
				break;
			case FLAG_RCV:
				if(buf == A || buf == 0x01){
					state_t = A_RCV;
				}
				else if(buf == FLAG)
					state_t = FLAG_RCV;
				else
					state_t = START;
				break;
			case A_RCV:
				if(buf == FLAG)
					state_t = FLAG_RCV;
				else {
					state_t = C_RCV;
					c = buf;
				}
				break;
			case C_RCV:
				if(buf == FLAG){
					state_t = FLAG_RCV;
				}
				else
					state_t = BCC_OK;
				break;
			case BCC_OK:
				if(buf == FLAG){
					state_t = STP;
				}
				else
					state_t = START;
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


int sendFrame_t(int fd, unsigned char *cmd, int size, int timeout, int numTries){ //retorno da funcao -> int consoante o byte C que recebeu (UA, RR, REJ, DISC, !sent)
    int sent = FALSE;
    alarmCount = 0;
    alarmEnabled = FALSE;
    state_t = START;
    
	int r = -1;
	(void)signal(SIGALRM, alarmHandler);
    while(alarmCount < numTries && !sent){
        if (alarmEnabled == FALSE){
        
            int bytes = write(fd, cmd, size);
            if(cmd[2] == UA) return 5; 
            printf("escrevi %d\n", bytes);

            alarm(timeout); // Set alarm to be triggered in 3s

            alarmEnabled = TRUE;
            
            
			r = receiveFrame_t(fd);
			
			if(state_t == STP){ //se tiver recebido retransmissão
				if(r != 3 && r != 4){sent = TRUE;}
				alarm(0);
				alarmEnabled = FALSE;
			}
			printf("alarmCount - %d\n", alarmCount);
           

        }
    }
    
    if(alarmCount == numTries){
    	r = 6; //numero de tentativas de envio excedidas, terminar execução (return na main)
    }
    
	return r;
}
