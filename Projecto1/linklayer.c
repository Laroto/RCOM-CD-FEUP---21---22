#include "linklayer.h"
#include "extras.h"

linkLayerState state;
volatile int timeout = FALSE;

void alarmHandler(int signal)
{
    //printf("timeout trigger\n");
    timeout = TRUE;
}

int llopen(linkLayer connectionParameters)
{
    (void)signal(SIGALRM, alarmHandler);

    if (setup_serial_port(connectionParameters, &state) == FAILURE)
    {
        return FAILURE;
    }

    state.role = connectionParameters.role;
    state.numTries = connectionParameters.numTries;
    state.timeOut = connectionParameters.timeOut;
    state.N = 0;
    state.last_request_address = 0;
    state.last_request_control = 0;
    
    state.n_retransmitted_frames = 0;
    state.n_received_i_frames = 0;
    state.n_timeouts = 0;
    state.n_rej = 0;

    if (connectionParameters.role == TRANSMITTER)
    {
        frame_t frame_set = construct_frame(A_TRANSMITTER, C_SET, NULL, 0);
        frame_t frame_ua;

        int send_count = 0;
        int received_ua = FALSE;
        timeout = TRUE;

        do
        {
            if (timeout && send_count < state.numTries)
            {
                if (write_frame(state, &frame_set) == FAILURE)
                {
                    close_serial_port(&state);
                    return FAILURE;
                }
                if(send_count != 0) {
                    state.n_timeouts++;
                    state.n_retransmitted_frames++;
                }
                send_count++;
                timeout = FALSE;
                printf("\tConnection Attempt: %d\n",send_count);
                alarm(state.timeOut);
            }

            if (read_frame(state, &frame_ua) == SUCCESS && frame_ua.address == A_TRANSMITTER && frame_ua.control == C_UA)
            {
                received_ua = TRUE;
                break;
            }
        } while (send_count < state.numTries || !timeout);

        alarm(0); // Cancel Alarm

        if (!received_ua)
        {
            close_serial_port(&state);
            return FAILURE;
        }
    }
    else if (connectionParameters.role == RECEIVER)
    {
        frame_t frame_set;

        do
        {
            if (read_frame(state, &frame_set) != SUCCESS)
            {
                continue;
            }
        } while (frame_set.address != A_TRANSMITTER || frame_set.control != C_SET);

        frame_t frame_ua = construct_frame(A_TRANSMITTER, C_UA, NULL, 0);
        if (write_frame(state, &frame_ua) == FAILURE)
        {
            close_serial_port(&state);
            return FAILURE;
        }

        state.last_response_frame = frame_ua;
        state.last_request_address = frame_set.address;
        state.last_request_control = frame_set.control;
    }
    else
    {
        return FAILURE;
    }

    return SUCCESS;
}

int llwrite(char *buf, int bufSize)
{
    if (buf == NULL || bufSize < 0)
    {
        return FAILURE;
    }

    if (bufSize > MAX_PAYLOAD_SIZE)
    {
        return FAILURE;
    }

    int c_rr = MAKE_RR(((state.N + 1) % 2));
    int c_rej = MAKE_REJ(state.N);
    char address = state.role == TRANSMITTER ? A_TRANSMITTER : A_RECEIVER;

    frame_t frame_i = construct_frame(address, MAKE_CONTROL(state.N), buf, bufSize);
    frame_t frame_resp;
    int send_count = 0;
    timeout = TRUE;
    int received_rr = FALSE;

    do
    {
        if (timeout && send_count < state.numTries)
        {
            if (write_frame(state, &frame_i) == FAILURE)
            {
                return FAILURE;
            }
            if (send_count != 0)
            {
                state.n_timeouts++;
                state.n_retransmitted_frames++;
            }
            send_count++;
            timeout = FALSE;
            printf("\tAttempt: %d\n",send_count);
            alarm(state.timeOut);
        }

        if (read_frame(state, &frame_resp) == SUCCESS && frame_resp.address == address)
        {
            if (frame_resp.control == c_rr)
            {
                received_rr = TRUE;
                break;
            }
            else if (frame_resp.control == c_rej)
            {
                timeout = TRUE;
                send_count = 0;
                state.n_retransmitted_frames++;
                continue;
            }
        }
    } while (send_count < state.numTries || !timeout);

    alarm(0);

    if (!received_rr)
    {
        return FAILURE;
    }

    state.N = (state.N + 1) % 2;

    return bufSize;
}

int llread(char *packet)
{
    if (packet == NULL)
    {
        return FAILURE;
    }

    char expected_address = state.role == TRANSMITTER ? A_RECEIVER : A_TRANSMITTER;
    int next_N = (state.N + 1) % 2;
    char expected_control = MAKE_CONTROL(state.N);
    int read_r;

    frame_t frame_i;

    timeout = FALSE;
    (void)signal(SIGALRM, alarmHandler);
    alarm(state.timeOut*(state.numTries+1));

    while (1)
    {
        if (timeout)
        {
            timeout = FALSE;
            alarm(0);
            printf("Recieving Timeout\n");
            break;
        }

        read_r = read_frame(state, &frame_i);
        if (read_r == FAILURE)
        {
            continue;
        }

        if (frame_i.control == state.last_request_control && frame_i.address == state.last_request_address)
        {
            if (write_frame(state, &state.last_response_frame) == FAILURE)
            {
                return FAILURE;
            }
            state.n_retransmitted_frames++;
            continue;
        }
        else if (frame_i.control == expected_control && frame_i.address == expected_address)
        {
            state.n_received_i_frames++;
            if (read_r == INCORRECT_DATA)
            {
                frame_t frame_rej = construct_frame(expected_address, MAKE_REJ(state.N), NULL, 0);
                if (write_frame(state, &frame_rej) == FAILURE)
                {
                    return FAILURE;
                }
                state.n_rej++;
                continue;
            }
            frame_t frame_rr = construct_frame(expected_address, MAKE_RR(next_N), NULL, 0);
            if (write_frame(state, &frame_rr) == FAILURE)
            {
                return FAILURE;
            }

            memcpy(packet, frame_i.data, frame_i.data_size * sizeof(char));
            state.N = next_N;

            state.last_response_frame = frame_rr;
            state.last_request_address = frame_i.address;
            state.last_request_control = frame_i.control;

            return frame_i.data_size;
        }
    }
    return FAILURE;
}

int llclose(int showStatistics)
{
    if (state.role == TRANSMITTER)
    {
        // DISC and then UA when receives disc from receiver
        frame_t frame_disc_t = construct_frame(A_TRANSMITTER, C_DISC, NULL, 0);
        frame_t frame_disc_r;
        frame_t frame_ua = construct_frame(A_RECEIVER, C_UA, NULL, 0);

        int send_count = 0;
        int received_disc_r = FALSE;
        timeout = TRUE;

        do
        {
            if (timeout && send_count < state.numTries)
            {
                if (write_frame(state, &frame_disc_t) == FAILURE)
                {   
                    return FAILURE;
                }
                if (send_count != 0)
                {
                    state.n_timeouts++;
                    state.n_retransmitted_frames++;
                }
                send_count++;
                timeout = FALSE;
                alarm(state.timeOut);
            }

            if (read_frame(state, &frame_disc_r) == SUCCESS)
            {
                if (frame_disc_r.address == A_RECEIVER && frame_disc_r.control == C_DISC)
                {
                    received_disc_r = TRUE;
                    break;
                }
            }
        } while (send_count < state.numTries || !timeout);

        alarm(0); // Cancel Alarm

        if (!received_disc_r)
        {
            close_serial_port(&state);
            return FAILURE;
        }
        
        if (write_frame(state, &frame_ua) == FAILURE)
        {
            close_serial_port(&state);
            return FAILURE;
        }
    }
    else if (state.role == RECEIVER)
    {
        frame_t frame_disc_r = construct_frame(A_RECEIVER, C_DISC, NULL, 0);
        frame_t frame_disc_t;
        frame_t frame_ua;
        int read_r;

        timeout = FALSE;
        (void)signal(SIGALRM, alarmHandler);
        alarm(state.timeOut*state.numTries);

        do
        {
            read_r = read_frame(state, &frame_disc_t);
            if (read_r == FAILURE)
            {
                continue;
            }
            
            if (frame_disc_t.control == state.last_request_control && frame_disc_t.address == state.last_request_address)
            {
                if (frame_disc_t.data != NULL) state.n_received_i_frames++;
                if (write_frame(state, &state.last_response_frame) == FAILURE)
                {
                    return FAILURE;
                }
                state.n_retransmitted_frames++;
                continue;
            }
            if (timeout == TRUE)
            {
                alarm(0);
                return FAILURE;
            }
            
        } while ((read_r != SUCCESS || frame_disc_t.address != A_TRANSMITTER || frame_disc_t.control != C_DISC) && !timeout);

        if (read_r == FAILURE)
        {
            return FAILURE;
        }

        int send_count = 0;
        timeout = TRUE;
        alarm(state.timeOut);
    
        do
        {
            if (timeout && send_count < state.numTries)
            {
                if (write_frame(state, &frame_disc_r) == FAILURE)
                {
                    return FAILURE;
                }
                if (send_count != 0)
                {
                    state.n_timeouts++;
                    state.n_retransmitted_frames++;
                }
                send_count++;
                timeout = FALSE;
                alarm(state.timeOut);
            }

            if (read_frame(state, &frame_ua) == SUCCESS && frame_ua.address == A_RECEIVER && frame_ua.control == C_UA)
            {
                break;
            }

        } while (send_count < state.numTries);

        alarm(0); // Cancel Alarm   
    }
    else
    {
        return FAILURE;
    }

    if (close_serial_port(&state) == FAILURE)
    {
        return FAILURE;
    }

    if (showStatistics == 1){
        printf("====Statistics====\n");
        printf("Number of retransmitted frames: %d\n", state.n_retransmitted_frames);
        printf("Number of received I frames: %d\n", state.n_received_i_frames);
        printf("Number of timeouts: %d\n", state.n_timeouts);
        printf("Number of sent REJs: %d\n", state.n_rej);
    }

    return SUCCESS;
}