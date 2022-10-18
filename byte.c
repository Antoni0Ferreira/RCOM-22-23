#include <stdio.h>
#include <stdlib.h>

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
	return j;
}


int main() {
	int size = 11;
	unsigned char result[11];
	unsigned char cmd[11] = {FLAG,A,SET,BCC,0x12, 0x14, 0x7E, 0x34, 0x7D,0x04,FLAG};
	int newSize = byteStuffing(cmd, size, result);
	
	printf("newSize - %d\n", newSize);
	for(int i = 0; i < 3; i++){
		printf("%x\n",result[i]);
	}
	
	return 0;
}
