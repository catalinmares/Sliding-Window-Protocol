#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "utils.h"

#define HOST "127.0.0.1"
#define PORT 10001

int main(int argc, char** argv) {
    init(HOST,PORT);
    
    /* message declarations */
    msg r, t;
    my_msg m;
    int res;

    /* file declarations */
    int fd;
    char filename[] = "recv_";
    int total_size = 0;
    int start;
    char* buffer = (char*) calloc(MAXFILESIZE, sizeof(char));

    /* frames declarations */
    int frames_to_recv = COUNT;  // initially 1000, will be updated later
    int frames_recv = 0;
    int msg_size;
    int recv_sequence;
    int checksum;

    while (frames_recv < frames_to_recv) {
        /* clean up message */
        memset(&r, 0, sizeof(msg));
        memset(&m, 0, sizeof(my_msg));
        
        /* receive frame from sender */
        res = recv_message(&r);

        if (res < 0) {
            perror("[RECEIVER] Receive error. Exiting.\n");
            exit(1);
        }

        m = *((my_msg*) r.payload);
        msg_size = r.len - 2 * sizeof(int);
        recv_sequence = m.sequence_number;
        checksum = calculate_checksum(m.payload, m.sequence_number, msg_size);

        /* check if correupted */
        if (m.checksum != checksum) {
            /* clean up message */
            memset(&t, 0, sizeof(msg));
            memset(&m, 0, sizeof(my_msg));
            
            /* send NAK to sender */
            sprintf(m.payload, "NAK");
            m.sequence_number = recv_sequence;
            t.len = strlen(m.payload) + 2 * sizeof(int);
            memcpy(t.payload, &m, sizeof(m));
            
            res = send_message(&t);
        
            if (res < 0) {
                perror("[RECEIVER] Send ACK error. Exiting.\n");
                exit(1);
            }

            continue;
        }

        switch (m.sequence_number) {
            case 0: // file name has sequence 0
                strcat(filename, m.payload);

                fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC);

                if (fd < 0) {
                    perror("[RECEIVER] File couldn't be created. Exiting.\n");
                    exit(1);
                }

                break;

            case 1: // numbr of frames has sequence 1
                frames_to_recv = atoi(m.payload);

                break;

            default:
                // write in the buffer at the correct position
                start = (m.sequence_number - 2) * MYMSGSIZE;
                memcpy(buffer + start, m.payload, msg_size);

                total_size += msg_size;
        }

        /* clean up message */
        memset(&t, 0, sizeof(msg));
        memset(&m, 0, sizeof(my_msg));
           
        /* send ACK to sender */ 
        sprintf(m.payload, "ACK");
        m.sequence_number = recv_sequence;
        t.len = strlen(m.payload) + 2 * sizeof(int);
        memcpy(t.payload, &m, sizeof(my_msg));

        frames_recv++;

        if (frames_recv == frames_to_recv) {
            write(fd, buffer, total_size);
            free(buffer);
            close(fd);

            res = send_message(&t);
            return 0;
        }
            
        res = send_message(&t);
        
        if (res < 0) {
            perror("[RECEIVER] Send ACK error. Exiting.\n");
            exit(1);
        }
    }

    return 0;
}
