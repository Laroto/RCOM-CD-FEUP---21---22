#include "linklayer.h"

int main(int argc, char** argv)
{
    if ( (argc < 2) || 
  	     ((strcmp("/dev/ttyS0", argv[1])!=0) && 
          (strcmp("/dev/ttyS10", argv[1])!=0) && 
          (strcmp("/dev/ttyS11", argv[1])!=0) && 
  	      (strcmp("/dev/ttyS1", argv[1])!=0) )) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      return(1);
    }

    linkLayer writer;
    writer.baudRate = BAUDRATE_DEFAULT;
    writer.numTries = MAX_RETRANSMISSIONS_DEFAULT;
    writer.role = TRANSMITTER;
    writer.timeOut = TIMEOUT_DEFAULT;
    strcpy(writer.serialPort,argv[1]);

    llopen(writer);
}