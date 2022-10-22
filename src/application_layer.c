// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

FILE *receptorFptr;

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{

	LinkLayer connectionParameters = {.baudRate = baudRate, .nRetransmissions = nTries, .timeout = timeout};
	strcpy(connectionParameters.serialPort, serialPort);
	
	if(strcmp(role,"tx") == 0){
			
			FILE *transmissorFptr = fopen(filename,"rb");
			
			if(transmissorFptr == NULL){
				printf("Can't open file!\n");
				exit(1);
			}
			
			fseek(transmissorFptr,0L,SEEK_END);
			
			
			
			LinkLayerRole llrole = LlTx;
			connectionParameters.role = llrole;
			
	   		int fd = llopen(connectionParameters);
	   		printf("depois do llopen()\n");
	   		long int fileSize = ftell(transmissorFptr);
	   		printf("file size - %ld\n", fileSize);
			long int auxFileSize = fileSize;
			unsigned char countBytes = 0;

			fseek(transmissorFptr,0L,SEEK_SET);

			while(auxFileSize > 0){
				countBytes++;
				auxFileSize /= 255;
			}

			unsigned char c = 2;
			unsigned char t1 = 0, l1 = countBytes;
			unsigned char *v1 = malloc(countBytes);

			for(int i = countBytes - 1; i >= 0; i--){
				v1[i] = (0xFF & fileSize);
				fileSize = fileSize >> 8;
			}

			unsigned char *controlPacket = malloc(3 + countBytes); // (c, t1, l1) + v1
			controlPacket[0] = c;
			controlPacket[1] = t1;
			controlPacket[2] = l1;

			for(int i = 3; i < 3 + countBytes; i++){
				controlPacket[i] = v1[i-3];
			}

			llwrite(controlPacket,3 + countBytes); //start
			printf("escrevi o start\n");

			unsigned char *buffer = malloc(500); // 4 + 496
	   		int bytes = 0;

			c = 1;
			unsigned char n = 0, l2 = 0x01; 
			l1 = 0xF0;

			buffer[0] = c;
			buffer[1] = n;
			buffer[2] = l2;
			buffer[3] = l1;

	   		while((bytes = fread(buffer + 4, 1, 496, transmissorFptr)) > 0){
	   			fseek(transmissorFptr, 496, SEEK_CUR); 
	   			llwrite(buffer, bytes + 4);
				n = (n + 1) % 255;
				buffer[1] = n;

	   		}
	   		//printf("bytes fora do while - %d\n", bytes);
	   		//if(bytes == 0){
	   		//	llwrite(buffer, bytes + 4);
	   		//}
	   		//llwrite end
	   		fclose(transmissorFptr);

			c = 3;
			controlPacket[0] = c;

			llwrite(controlPacket, 3 + countBytes); //end

	   		llclose(fd);
	}
	else if(strcmp(role,"rx") == 0){

			LinkLayerRole llrole = LlRx;
			connectionParameters.role = llrole;
			unsigned char *packet = malloc(1000);
	   		int fd = llopen(connectionParameters);

			llread(packet);
			
			printf("li o start, packet[0] = %x\n", packet[0]);

			receptorFptr = fopen(filename, "w");

			int ended = 0;

	   		//llread start
	   		while(!ended){ //enquanto nao receber o end
	   			ended = llread(packet);
	   			printf("ended  - %d\n", ended);
				fwrite(packet + 4, 1, 496, receptorFptr);
	   		}
	   		llclose(fd);
	}    
}
