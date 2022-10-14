// Read from serial port in non-canonical mode
//
// Modified by: Eduardo Nuno Almeida [enalmeida@fe.up.pt]

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

// Baudrate settings are defined in <asm/termbits.h>, which is
// included by <termios.h>
#define BAUDRATE B38400
#define _POSIX_SOURCE 1 // POSIX compliant source

#define FALSE 0
#define TRUE 1

#define BUF_SIZE 256

#define FLAG (0x7E)
#define A (0x03)
#define SET (0x03)
#define UA (0x07)
#define BCC (A^UA)
#define DISC (0x0B)

volatile int STOP = FALSE;
<<<<<<< HEAD
enum STATE {START, FLAG_RCV, A_RCV, C_RCV, BCC_OK, STP};

enum STATE state = START;


int receiveFrame(int fd, unsigned char *fr_a, unsigned char *fr_c){
    
    
    while(state != STP){
        unsigned char buf; 

        read(fd, &buf, 1);
        if(buf != 0x00 && buf != 0x7e){
            printf("%x\n", buf);
        }
        
        switch (state) {
=======
enum STATE = {START, FLAG_RCV, A_RCV, C_RCV, BCC_ok, STOP};

enum STATE state = START;

int receiveFrame(unsigned char &a, unsigned char &c){
    
    
    while(state != STOP){
        unsigned char buf; 

        read(fd, &buf, 1);
        
        switch (STATE) {
>>>>>>> 955aaa44c095bcb4e92fdbc6bc977c8fa4d36df4
            case START:
                if(buf == FLAG)
                    state = FLAG_RCV;
                break;
            case FLAG_RCV:
                if(buf == A){
                    state = A_RCV;
<<<<<<< HEAD
                    (*fr_a) = buf;
=======
                    a = buf;
>>>>>>> 955aaa44c095bcb4e92fdbc6bc977c8fa4d36df4
                }
                else if(buf == FLAG)
                    state = FLAG_RCV;
                else
                    state = START;
                break;
            case A_RCV:
<<<<<<< HEAD
                if( TRUE){
                    state = C_RCV;
                    (*fr_c) = buf;
=======
                if( /*C VÁLIDO*/){
                    state = C_RCV;
                    c = buf;
>>>>>>> 955aaa44c095bcb4e92fdbc6bc977c8fa4d36df4
                }
                else if(buf == FLAG)
                    state = FLAG_RCV;
                else
                    state = START;
                break;
            case C_RCV:
                if(buf == FLAG){
<<<<<<< HEAD
                    state = FLAG_RCV;
                }
                else
                    state = BCC_OK;
                break;
            case BCC_OK:
                if(buf == FLAG)
                    state = STP;
=======
                    
                }
                break;
            case BCC_OK:
                if(buf == FLAG)
                    state = STOP;
>>>>>>> 955aaa44c095bcb4e92fdbc6bc977c8fa4d36df4
                else
                    state = START;
                break;
            
        }
<<<<<<< HEAD
        
=======
            
        if(memcmp(buf[0], FLAG,1) || memcmp(buf[4], FLAG,1)){
            
            //TERMINAR
        }
        if(memcmp(buf[1] ^ buf[2], buf[3],1) != 0){
            //TERMINAR
        }
        switch (buf[2]){
            case SET: //iniciar conexao, responder UA
                
                write(fd, cmd, 5);
                break;
                
            case UA: //deu erro (?)
                
                break;
            case DISC: //TERMINAR CONEXAO
                disconnect = TRUE;
                write(fd, cmd, 5);
                break;
            
        }
>>>>>>> 955aaa44c095bcb4e92fdbc6bc977c8fa4d36df4
    }
}

int main(int argc, char *argv[])
{
    // Program usage: Uses either COM1 or COM2
    const char *serialPortName = argv[1];

    if (argc < 2)
    {
        printf("Incorrect program usage\n"
               "Usage: %s <SerialPort>\n"
               "Example: %s /dev/ttyS1\n",
               argv[0],
               argv[0]);
        exit(1);
    }

    // Open serial port device for reading and writing and not as controlling tty
    // because we don't want to get killed if linenoise sends CTRL-C.
    int fd = open(serialPortName, O_RDWR | O_NOCTTY);
    if (fd < 0)
    {
        perror(serialPortName);
        exit(-1);
    }

    struct termios oldtio;
    struct termios newtio;

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
    newtio.c_cc[VTIME] = 0; // Inter-character timer unused
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

    printf("New termios structure set\n");
    int disconnect = FALSE;
    unsigned char cmd[] = {FLAG,A,UA,BCC,FLAG};
    int i = 0;
    while(i < 3){

<<<<<<< HEAD
        unsigned char a,c;
        receiveFrame(fd, &a, &c);
        state = START;
        i++;
        /*if(c == SET){
            write(fd, cmd, 5);
        }  */
=======
        //receiveFrame()  // se retornar -1 (?) é disconnect
        
        
>>>>>>> 955aaa44c095bcb4e92fdbc6bc977c8fa4d36df4
    
    }

    // The while() cycle should be changed in order to respect the specifications
    // of the protocol indicated in the Lab guide

    // Restore the old port settings
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    close(fd);

    return 0;
}
