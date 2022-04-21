#ifndef _EXTRAS_H
#define _EXTRAS_H

#include "linklayer.h"
#include <stdlib.h>

#define FLAG 0x7E
#define A_TRANSMITTER 0x05
#define A_RECEIVER 0x02
#define C_SET 0x03
#define C_UA 0x07
#define C_DISC 0x0B

#define ESCAPE 0x7D
#define STUFFING_BYTE 0x20

#define DEBUG TRUE
#define SUCCESS 1
#define FAILURE -1
#define INCORRECT_DATA -2

#define MAKE_CONTROL(S) S << 1
#define MAKE_RR(R) (R << 5) | 0b1
#define MAKE_REJ(R) (R << 5) | 0b101

typedef struct frame_t
{
    char address;
    char control;
    char data[MAX_PAYLOAD_SIZE];
    int data_size;
    char buffer[MAX_PAYLOAD_SIZE * 2 + 10];
    int buffer_size;
} frame_t;

typedef struct linkLayerState
{
    int fd;   /* File descriptor corresponding to serial port */
    int role; /* TRANSMITTER | RECEIVER */
    int numTries;
    int timeOut;
    int N;
    frame_t last_response_frame;
    char last_request_address;
    char last_request_control;

    int n_retransmitted_frames;
    int n_received_i_frames;
    int n_timeouts;
    int n_rej;
} linkLayerState;

void alarmHandler(int signal);

int setup_serial_port(linkLayer connectionParameters, linkLayerState *state);
int close_serial_port(linkLayerState *state);
int get_termios_baud(int baud);

frame_t construct_frame(char address, char control, char *data, int data_size);
int deconstruct_frame(frame_t *frame);
int read_frame(linkLayerState state, frame_t *frame);
int write_frame(linkLayerState state, frame_t *frame);

#endif