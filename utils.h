#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "link_emulator/lib.h"

#define MYMSGSIZE 1392
#define MAXFILESIZE 1048576
#define COUNT 1000

typedef struct {
	int checksum;
	int sequence_number;
	char payload[MYMSGSIZE];
} my_msg;

int calculate_checksum(char* payload, int sequence_number, int len) {
	int i;
	char* byte;
	int checksum = 0;

    for (i = 0; i < len; i++) {
        checksum ^= payload[i];
    }

    byte = (char*) (&sequence_number);

    for (i = 0; i < 4; i++) {
    	checksum ^= *(byte + i);
    }

    return checksum;
}