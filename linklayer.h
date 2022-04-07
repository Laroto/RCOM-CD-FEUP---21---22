#ifndef LINKLAYER
#define LINKLAYER

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

typedef struct linkLayer{
    char serialPort[50];
    int role; //defines the role of the program: 0==Transmitter, 1=Receiver
    int baudRate;
    int numTries;
    int timeOut;
    struct termios oldtio,newtio;
} linkLayer;

//ROLE
#define NOT_DEFINED -1
#define TRANSMITTER 0
#define RECEIVER 1


//SIZE of maximum acceptable payload; maximum number of bytes that application layer should send to link layer
#define MAX_PAYLOAD_SIZE 1000

//CONNECTION deafault values
#define BAUDRATE_DEFAULT B38400
#define MAX_RETRANSMISSIONS_DEFAULT 3
#define TIMEOUT_DEFAULT 4
#define _POSIX_SOURCE 1 /* POSIX compliant source */

//MISC
#define FALSE 0
#define TRUE 1

//flags
#define C_writer 0x03
#define C_disconnect 0x0B
#define C_reader 0x07
#define FLAG 0x7E
#define A_transmissor 0x03
#define A_recetor 0x01
#define BCC_write 0x06
#define BCC_reader 0x00

volatile int STOP=FALSE;
volatile unsigned char flag_tentativas=1;
volatile unsigned char flag_alarme=1;
volatile unsigned char flag_error=0;

#define TRAMA_I 1
#define TRAMA_S 0

// Opens a conection using the "port" parameters defined in struct linkLayer, returns "-1" on error and "1" on sucess
int llopen(linkLayer connectionParameters);
// Sends data in buf with size bufSize
int llwrite(char* buf, int bufSize);
// Receive data in packet
int llread(char* packet);
// Closes previously opened connection; if showStatistics==TRUE, link layer should print statistics in the console on close
int llclose(int showStatistics);


//processa a trama para verificar se está correta
void estados(unsigned char c, int *state, unsigned char* trama, int* tamanho, int tipo);
//atende a interrupção de timeout de resposta
void atende();
#endif