#include "linklayer.h"
#include "linkextra.h"

volatile int timeout = FALSE;
linkLayerState state;

void alarmHandler(int signal)
{
    timeout = TRUE;
}

int llopen(linkLayer connectionParameters)
{
    (void)signal(SIGALRM, alarmHandler);

    if (setup_serial_port(connectionParameters) == -1)
    {
        return -1;
    }

    state.role = connectionParameters.role;
    state.numTries = connectionParameters.numTries;
    state.timeOut = connectionParameters.timeOut;
    state.numTries = 0;
    
    if (connectionParameters.role == TRANSMITTER)
    {
        frame_t set = construct_frame(A_TRANSMITTER, C_SET, NULL, 0); 
        frame_t ua;

        int count = 0;
        int gotit = FALSE;
        timeout = TRUE;

        do
        {
            if (timeout)
            {
                if (write_frame(state,&set) == -1)
                {
                    end_port(&state);
                    return -1;
                }

                if (count != 0)
                {
                    state.n_timeouts ++;
                    state.n_retransmited ++;
                }
                count ++;
                timeout = FALSE;
                alarm(state.timeOut);
            }

            if (read_frame(state,&ua) == 1 && ua.address == A_TRANSMITTER && ua.control == C_UA)
            {
                gotit = TRUE;
                break;
            }

        } while (count < state.numTries || !timeout);

        alarm(0);

        if (!gotit)
        {
            end_port(&state);
            return -1;
        }

    }
    else if (connectionParameters.role == RECEIVER)
    {
        frame_t set;

        do
        {
            if (read_frame(state,&set) != 1)
            {
                continue;
            }

        } while (set.address != A_TRANSMITTER || set.control != C_SET);

        frame_t ua = construct_frame(A_TRANSMITTER, C_UA, NULL,0);

        if (write_frame (state,&ua) == -1)
        {
            end_port(&state);
        }

        state.last_response_frame = ua;
        state.last_request_address = set.address;
        state.last_request_control = set.control;
    }
    else
    {
        return -1;
    }
}