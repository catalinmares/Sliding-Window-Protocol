#include <stdio.h>
#include <stdlib.h>
#include "link_emulator/lib.h"

typedef struct node {
	msg data;
	int seq_no;
	struct node *next;
} TList;

TList* create_list();

void insert(TList** head, msg data, int seq_no);

msg delete(TList** head, int seq_no);

void print_list(TList* head);

void destroy_list(TList** head);