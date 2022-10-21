// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{

	LinkLayer connectionParameters = {.serialPort = serialPort, .baudRate = baudRate, 
								  .nRetransmissions = nTries, .timeout = timeout};
	if((*role) == "tx"){
		
			FILE fptr = fopen(filename,"rb");
			
			if(fptr == NULL){
				printf("Can't open file!\n");
				exit();
			}
			LinkLayerRole llrole = LlTx;
			connectionParameters.role = llrole;

	   		int fd = llopen(connectionParameters);
	   		
	   		
	   		unsigned char *buffer = malloc(500);
	   		int bytes = 0;
	   		//llwrite start
	   		while((bytes = fread(buffer, 1, 500, fptr)) == 500){
	   			fptr += 500;
	   			llwrite(buffer, bytes);
	   		}
	   		if(bytes > 0){
	   			llwrite(buffer, bytes);
	   		}
	   		//llwrite end
	   		fclose(fptr);
	   		llclose(fd);
	}
	else if((*role) == "rx"){
			LinkLayerRole llrole = LlRx;
			connectionParameters.role = llrole;
			unsigned char *packet = malloc(1000);
	   		int fd = llopen(connectionParameters);
	   		//llread start
	   		while(!ended){ //enquanto nao receber o end
	   			llread(packet);
	   		}
	   		llclose(fd);
	}    
}
