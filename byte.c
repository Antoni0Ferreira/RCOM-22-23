#include <stdio.h>
#include <stdlib.h>

#define FALSE 0
#define TRUE 1

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


int main() {
	int size = 11;
	unsigned char *result = malloc(size);
	
	unsigned char cmd[11] = {FLAG,A,SET,BCC,0x12, 0x14, 0x7E, 0x34, 0x7D,0x04,FLAG};
	int newSize = byteStuffing(cmd, size, result);
	
	unsigned char *result2 = malloc(newSize);
	printf("newSize - %d\n", newSize);
	for(int i = 0; i < newSize; i++){
		printf("%x\n",result[i]);
	}
	
	int newSize2 = destuffing(result, newSize, result2);
	printf("newSize2 - %d\n", newSize2);
	for(int i = 0; i < newSize2; i++){
		printf("%x\n",result2[i]);
	}
	
	return 0;
}

