#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "utils.h"
#include "list.h"

#define HOST "127.0.0.1"
#define PORT 10000

int main(int argc, char** argv){
  	init(HOST,PORT);
  	
    /* message declarations */
  	msg t, r;
  	my_msg m;
    int res;
  	
    /* file declarations */
  	int fd;
  	struct stat file_data;
  	int file_size;
    int bytes_read = 0;

    /* frames declarations*/
  	int frames_to_send;
    int frames_sent;
  	int seq_no = 0;
  	int msg_size;

    /* window declarations */
    int BDP;
    int window_capacity;
    int window_size = 0;
    int timeout;

    /* list declarations */
    TList* unconfirmed;
  	TList* current;

    /* setup timeout */
    if (atoi(argv[3]) <= 10) {
        timeout = 4 * atoi(argv[3]);
    } else {
        timeout = 2 * atoi(argv[3]);
    }


  	/* calculate window capacity */
  	BDP = atoi(argv[2]) * atoi(argv[3]) * 1000;
  	window_capacity = BDP / (MSGSIZE * 8);

  	/* open file */
  	fd = open(argv[1], O_RDONLY);

  	if (fd < 0) {
  		perror("[SENDER] Failed opening file specified. Exiting.\n");
  		exit(1);
  	}

  	/* calculate file size */
    fstat(fd, &file_data);
    file_size = file_data.st_size;

    /* calculate number of frames to send */
    frames_to_send = (int) ceil((float) file_size / MYMSGSIZE) + 2;

    /* create list for frames sent and unconfirmed */
    unconfirmed = create_list();

    /* send file name to receiver */
	sprintf(m.payload, "%s", argv[1]);
	m.sequence_number = seq_no++;
    m.checksum = calculate_checksum(m.payload, m.sequence_number, (int) strlen(m.payload));

    memcpy(t.payload, &m, sizeof(my_msg));
    t.len = strlen(m.payload) + 2 * sizeof(int);

    insert(&unconfirmed, t, m.sequence_number);

    res = send_message(&t);

    if (res < 0) {
    	perror("[SENDER] Send error. Exiting.\n");
    }

    /* clean up message */
    memset(&m, 0, sizeof(my_msg));
    memset(&t, 0, sizeof(msg));

    /* send number of frames to expect to receiver */
    sprintf(m.payload, "%d", frames_to_send);
    m.sequence_number = seq_no++;
    m.checksum = calculate_checksum(m.payload, m.sequence_number, (int) strlen(m.payload));

    memcpy(t.payload, &m, sizeof(my_msg));
    t.len = strlen(m.payload) + 2 * sizeof(int);

    insert(&unconfirmed, t, m.sequence_number);

    res = send_message(&t);

    if (res < 0) {
    	perror("[SENDER] Send error. Exiting.\n");
    }

    window_size = 2;
    frames_sent = 2;

    while (window_size < window_capacity && bytes_read < file_size) {
    	/* clean up message */
    	memset(&m, 0, sizeof(my_msg));
    	memset(&t, 0, sizeof(msg));

        /* read file content */
    	msg_size = read(fd, m.payload, MYMSGSIZE);

    	m.sequence_number = seq_no++;
    	m.checksum = calculate_checksum(m.payload, m.sequence_number, msg_size);

    	memcpy(t.payload, &m, sizeof(my_msg));
    	t.len = msg_size + 2 * sizeof(int);

    	insert(&unconfirmed, t, m.sequence_number);
    	window_size++;

        /* send file content */
    	res = send_message(&t);

    	if (res < 0) {
    		perror("[SENDER] Send error. Exiting.\n");
    	}

    	frames_sent++;
        bytes_read += msg_size;
    }

    while (frames_sent < frames_to_send) {
    	/* clean up message */
    	memset(&t, 0, sizeof(msg));
        memset(&m, 0, sizeof(my_msg));

    	/* receive ACK from receiver */
    	res = recv_message_timeout(&r, timeout);

        /* reply received */
        if (res != -1) {
            m = *((my_msg*) r.payload);

            /* reply was ACK */
            if (strcmp(m.payload, "ACK") == 0) {
                delete(&unconfirmed, m.sequence_number);
                window_size--;
            }

            /* for reply NAK, just send a new frame */

            /* clean up message */
            memset(&t, 0, sizeof(msg));
            memset(&m, 0, sizeof(my_msg));

            /* read file content */
            msg_size = read(fd, m.payload, MYMSGSIZE);
            
            m.sequence_number = seq_no++;
            m.checksum = calculate_checksum(m.payload, m.sequence_number, msg_size);
        
            memcpy(t.payload, &m, sizeof(my_msg));
            t.len = msg_size + 2 * sizeof(int);

            insert(&unconfirmed, t, m.sequence_number);
            window_size++;

            /* send file content */
            res = send_message(&t);

            if (res < 0) {
                perror("[SENDER] Send error. Exiting.\n");
                exit(1);
            }

            frames_sent++;
 
        /* no reply from receiver */
        } else {
            /* there are no more frames on the window */
            /* resend the unACKed from the list */
            current = unconfirmed;
    
            while (current != NULL) {
                send_message(&current->data);
                current = current->next;
            }
        }
    }

    /* receive remaining ACKs */
    while (window_size > 0) {
    	/* clean up message */
    	memset(&m, 0, sizeof(my_msg));
    	memset(&t, 0, sizeof(msg));

    	/* receive ACK from receiver */
    	res = recv_message_timeout(&r, timeout);

        /* reply received */
    	if (res != -1) {
            m = *((my_msg*) r.payload);

            /* reply was ACK */
            if (strcmp(m.payload, "ACK") == 0) {
                delete(&unconfirmed, m.sequence_number);
                window_size--;
            }

            /* do nothing for reply NAK */

        /* no reply from receiver */
        } else {
            /* there are no more frames on the window */
            /* resend the unACKed from the list */
            current = unconfirmed;
    
            while (current != NULL) {
                send_message(&current->data);
                current = current->next;
            }
        }
    }

    close(fd);

  	return 0;
}
