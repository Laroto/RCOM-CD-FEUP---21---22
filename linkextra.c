#include "linkextra.h"

struct termios newtio, oldtio;

int set_port(linkLayer connectionParameters, linkLayerState *state)
{
    state->fd = open(connectionParameters.serialPort, O_RDWR | O_NOCTTY);
    if (state->fd < 0)
    {
        return -1;
    }

    if (tcgetattr(state->fd, &oldtio) == -1)
    {
        return -1;
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = get_termios_baud(connectionParameters.baudRate) | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    newtio.c_lflag = 0;

    newtio.c_cc[VTIME] = 2;
    newtio.c_cc[VMIN] = 0;

    tcflush(state->fd, TCIOFLUSH);

    if (tcsetattr(state->fd, TCSANOW, &newtio) == -1)
    {
        return -1;
    }

    return 1;
}

int get_termios_baud(int baud)
{
    switch (baud)
    {
    case 9600:
        return B9600;
    case 19200:
        return B19200;
    case 38400:
        return B38400;
    case 57600:
        return B57600;
    case 115200:
        return B115200;
    case 230400:
        return B230400;
    case 460800:
        return B460800;
    case 500000:
        return B500000;
    case 576000:
        return B576000;
    case 921600:
        return B921600;
    case 1000000:
        return B1000000;
    case 1152000:
        return B1152000;
    case 1500000:
        return B1500000;
    case 2000000:
        return B2000000;
    case 2500000:
        return B2500000;
    case 3000000:
        return B3000000;
    case 3500000:
        return B3500000;
    case 4000000:
        return B4000000;
    default:
        return B9600;
    }
}

int end_port(linkLayerState *state)
{
    if (tcsetattr(state->fd, TCSANOW, &oldtio) == -1)
    {
        return -1;
    }

    close(state->fd);
    return 1;
}

int deconstruct_frame (frame_t *frame)
{
    if (frame == NULL)
    {
        return -1;
    }

    if (frame->buffer_size < 5 || frame->buffer_size == 6)
    {
        return -1;
    }

    if (frame->buffer[0] != FLAG || frame->buffer[frame->buffer_size -1] != FLAG)
    {
        return -1;
    }

    char buffer[2 * MAX_PAYLOAD_SIZE +10];
    int buffer_size = 0;

    for(int i=0; i<frame->buffer[i]; i++)
    {
        if (frame->buffer[i] == ESCAPE)
        {
            i++;
            buffer[buffer_size] = frame->buffer[i] ^ STUFFING_BYTE;
        }
        else
        {
            buffer[buffer_size] = frame->buffer[i];
        }
        buffer_size ++;
    }

    frame->address = buffer[1];
    frame->control = buffer[2];
    int bcc1 = frame->address ^ frame->control;

    if (bcc1 != buffer[3])
    {
        return -1;
    }

    if (buffer_size > 5) 
    {
        frame->data_size = buffer_size - 6;
        memcpy(frame->data, buffer + 4, frame->data_size * sizeof(char));
        
        char bcc2 = 0;
        for (int i = 0; i < frame->data_size; i++)
        {
            bcc2 ^= frame->data[i];
        }
        if (bcc2 != buffer[buffer_size - 2])
        {
            return -2;
        }
    }
    else
    {
        frame->data_size = 0;
    }
    return 1;
}

frame_t construct_frame(char address, char control, char *data, int data_size)
{
    frame_t frame;

    frame.address = address;
    frame.control = control;

    if(data == NULL || data_size == 0)
    {
        frame.data_size = 0;
    }
    else
    {
        memcpy(frame.data,data,data_size);
        frame.data_size = data_size;
    }

    char buffer[MAX_PAYLOAD_SIZE + 4];
    int buffer_size = 3;

    buffer[0] = frame.address;
    buffer[1] = frame.control;
    buffer[2] = frame.address ^ frame.control;

    if (data != NULL)
    {
        //coisas de informação
    }

    frame.buffer[0] = FLAG;
    frame.buffer_size = 1;

    for(int i=0; i< buffer_size; i++)
    {
        if (buffer[i] == FLAG || buffer[i] == ESCAPE)
        {
            frame.buffer[frame.buffer_size] = ESCAPE;
            frame.buffer[frame.buffer_size + 1] = buffer[i] ^ STUFFING_BYTE;
            frame.buffer_size += 2;
        }
        else
        {
            frame.buffer[frame.buffer_size++] = buffer[i];
        }
    }

    frame.buffer[frame.buffer_size++] = FLAG;

    return frame;
}

int read_frame(linkLayerState state, frame_t *frame)
{
    frame->buffer_size = 0;

    char *buf = frame->buffer;

    do
    {
        if (read(state.fd, buf + frame->buffer_size, sizeof(char)) == 0)
        {
            return -1;
        }
        if (buf[frame->buffer_size] == FLAG || frame->buffer_size > 0)
        {
            frame->buffer_size ++;
        }

    } while (frame->buffer_size < 2 * MAX_PAYLOAD_SIZE + 10 && (frame->buffer_size < 2 || frame->buffer[frame->buffer_size - 1] != FLAG));

    return deconstruct_frame(frame);
}

int write_frame(linkLayerState state, frame_t *frame)
{
    int written = write(state.fd, frame->buffer, frame->buffer_size * sizeof(char));
    return written == frame->buffer_size ? 1 : -1;
}