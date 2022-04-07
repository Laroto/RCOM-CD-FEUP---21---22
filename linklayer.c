#include "linklayer.h"

int tentativas;
volatile int STOP=FALSE;
volatile unsigned char flag_tentativas=1;
volatile unsigned char flag_alarme=1;
volatile unsigned char flag_error=0;

void atende()
{
    flag_tentativas++;
    if (flag_tentativas >= tentativas)
    {
        flag_error = 1;
    }
    flag_alarme = 1;
}

void estados(unsigned char c, int *state, unsigned char* trama, int* tamanho, int tipo)
{
    switch (*state)
    {
        case 0: //start/ valida FLAG
            //printf("estado 0\n");
            if (c == FLAG)
            {
                trama[*tamanho] = c;
                tamanho++;
                *state = 1;
            }
            break;
        
        case 1: //valida A, C, BCC
            //printf("estado 1\n");
            if (c != FLAG)
            {
                trama[*tamanho] = c;
                tamanho++;

                if (*tamanho == 4)
                {
                    if ((trama[1]^trama[2]) != trama[3]) // ha erro de certeza
                        *state = -1;
                    else
                        *state = 2;
                } 
            }
            else
            {
                *tamanho = 1;
                trama[0] = c;
            }
            break;
        
        case 2: //sucesso
            //printf("estado 2\n");
            trama[*tamanho] = c;
            if(c == FLAG)
            {
                STOP = TRUE;
                alarm(0);
                flag_alarme = 0;
            }
            else
            {
                if (tipo == TRAMA_S)
                {
                    *state = 0;
                    *tamanho = 0;
                }
            }
            break;
        
        case -1: //caso de erro
            //printf("estado -1\n");
            trama[*tamanho-1] = c;
            if (c == FLAG)
            {
                if (tipo == TRAMA_I)
                {
                    flag_error = 1;
                    STOP = TRUE;
                }
                else
                {
                    *state = 1;
                    *tamanho = 1;
                }
            }
            break;
    }
}

int llopen(linkLayer connectionParameters)
{
    //prepara porta
    int fd = open(connectionParameters.serialPort, O_RDWR | O_NOCTTY );

    if (fd <0) 
    {
        perror(connectionParameters.serialPort); 
        return(-1); 
    }

    if (tcgetattr(fd,&connectionParameters.oldtio) == -1)  /* save current port settings */
    {
      perror("tcgetattr");
      return(-1);
    }

    bzero(&connectionParameters.newtio, sizeof(connectionParameters.newtio));
    connectionParameters.newtio.c_cflag = connectionParameters.baudRate | CS8 | CLOCAL | CREAD;
    connectionParameters.newtio.c_iflag = IGNPAR;
    
    connectionParameters.newtio.c_lflag = 0; /* set input mode (non-canonical, no echo,...) */

    connectionParameters.newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    connectionParameters.newtio.c_cc[VMIN]     = 1;   /* blocking read until 1 chars received */

    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&connectionParameters.newtio) == -1) 
    {
      perror("tcsetattr");
      return(-1);
    }
    ///////////////////////// 

    unsigned char trama[5], sinal[5];
    int state = 0;
    int tamanho = 0;
    int res;
    unsigned char c;

    if (connectionParameters.role) //recetor
    {
        unsigned char sinal[5] = {FLAG,A_recetor ,C_reader ,BCC_reader, FLAG};

        while (STOP==FALSE) 
        {       
            res = read(fd,&c,1);

            if(res>0)
            {
                tamanho++;
                estados(c, &state, trama, &tamanho, TRAMA_S);
            }
        }
        res = write(fd, sinal,5);
        return TRUE;
    }
    else //transmissor
    {
        unsigned char sinal[5] = {FLAG,A_transmissor ,C_writer ,BCC_write, FLAG};

        (void) signal(SIGALRM, atende);  // instala  rotina que atende interrupcao
        tentativas = connectionParameters.numTries;

        while (flag_tentativas < connectionParameters.numTries && flag_alarme == 1)
        {
            res = write(fd,sinal,5);
            alarm(connectionParameters.timeOut);

            while (STOP==FALSE)  //espera resposta UA
            {       
                res = read(fd,&c,1);
                flag_alarme = 0;
                if(res>0)
                {
                    tamanho++;
                    estados(c, &state, trama, &tamanho, TRAMA_S);
                }
            }
        }
        if (flag_error)
        {
            return FALSE;
        }
        else
        {
            return TRUE;
        }
    }    
}
