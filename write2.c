/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>

#include <string.h>

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

//flags
#define C_writer 0x03
#define C_disconnect 0x0B
#define C_reader 0x07
#define FLAG
#define A_transmissor 0x03
#define A_recetor 0x01
#define BCC_write 0x06
#define BCC_reader 0x00

volatile int STOP=FALSE;

void estados(unsigned char c, int *state, unsigned char* trama, int* length, int trama_type)
{
    switch (*state)
    {
        case 0: //start/ valida FLAG
        {
            if (c == FLAG)
            {
                trama[*length] = c;
                length++;
                state = 1;
            }
            break;
        }
        case 1: //valida A
        {
            if (c != FLAG)
            {
                trama[*length] = c;
                length++;

                if (length == 4)
                {
                    if (trama[1]^trama[2] != trama[3]) // ha erro de certeza
                    {

                    }
                } 
            }
            break;
        }
    };
}

int main(int argc, char** argv)
{
    int fd,c, res;
    struct termios oldtio,newtio;
    char buf[255];
    char *address= argv[1];
    int i, sum = 0, speed = 0;

    if ( (argc < 2) || 
  	     ((strcmp("/dev/ttyS0", argv[1])!=0) && 
  	      (strcmp("/dev/ttyS1", argv[1])!=0) )) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      return(1);
    }

// open port read+write
    fd = open(argv[1], O_RDWR | O_NOCTTY );
    if (fd <0) {perror(argv[1]); return(-1); }

    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
      perror("tcgetattr");
      return(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 5;   /* blocking read until 5 chars received */
////////////////////////


    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      return(-1);
    }


// read from keyboard and write
    printf("New termios structure set\n");

    scanf("%s",buf);
    int counter = 0;
    while(buf[counter]!=NULL)
	{
		counter++;
	}
    //write
    res = write(fd,buf,counter+1);   
    printf("%d Characters written\n", res);
/////////////

   
    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      return(-1);
    }

//read
	char output[255];
	counter = 0;
	int STOP = FALSE;
	
	while (STOP == FALSE)
	{
		res = read(fd,buf,1);
		output[counter] = buf[0];
		counter++;
		if(buf[0] == '\0') STOP = TRUE;
	}
	
	printf("Got %d characters!\n",counter);
	printf("%s\n",output);
////////////////////

//close
    close(fd);
}   