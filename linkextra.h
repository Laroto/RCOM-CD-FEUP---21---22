#ifndef _LINKEXTRA_H_
#define _LINKEXTRA_H_

#include "linklayer.h"

#define FLAG 0x7E
#define A_TRANSMITTER 0x05
#define A_RECEIVER 0x02
#define C_SET 0x03
#define C_UA 0x07
#define C_DISC 0x0B
#define ESCAPE 0x7D
#define STUFFING_BYTE 0x20

typedef struct linkLayerState
{
    int fd;
    int role; 
    int numTries;
    int timeOut;

    int n_retransmited;
    int n_recieved;
    int n_rej;
    int n_timeouts;

    frame_t last_response_frame;
    char last_request_address;
    char last_request_control;

} linkLayerState;

typedef struct frame_t
{
    char address;
    char control;
    char buffer[MAX_PAYLOAD_SIZE * 2 + 10];
    int buffer_size;
    int data_size;
    int data[MAX_PAYLOAD_SIZE];
} frame_t;

/*
* @brief configures serial port
* @return 1 ->sucess | -1 ->error
*/
int set_port(linkLayer connectionParameters, linkLayerState *state);

/*
* @brief closes serial port connection
* @return 1 ->sucess | -1 ->error
*/
int end_port(linkLayerState *state);

/*
*   @brief adds a 'B' to the baudrate
*/
int get_termios_baud(int baud);

/*
*   @brief constructs the frames to astablish connection
*/
frame_t construct_frame(char address, char control, char *data, int data_size);

/*
*   @brief destroys frame, managing memory
*/
int deconstruct_frame(frame_t *frame);

/*
*   @brief reads a frame
*/
int read_frame(linkLayerState state, frame_t *frame);

/*
*   @brief writes a frame 
*/
int write_frame(linkLayerState state, frame_t *frame);

#endif