#include <stdio.h>
#include <stdlib.h>
#include "list.h"

TList* create_list() {
	return NULL;
}

void insert(TList** head, msg data, int seq_no) {
    TList* temp, *prev, *next;
    
    temp = (TList*) calloc(1, sizeof(TList));
    temp->data = data;
    temp->seq_no = seq_no;
    temp->next = NULL;
    
    if (*head == NULL) {
        *head = temp;
    } else {
        prev = NULL;
        next = *head;

        while (next != NULL && next->seq_no < seq_no) {
            prev = next;
            next = next->next;
        }

        if (next == NULL) {
            prev->next = temp;
        } else {
            if (prev != NULL) {
                temp->next = prev->next;
                prev->next = temp;
            } else {
                temp->next = *head;
                *head = temp;
            }            
        }   
    }
}

msg delete(TList** head, int seq_no) {
	TList* current = *head;
	TList* prev = NULL;
	msg ret;

	do {
		if (current->seq_no == seq_no) {
			break;
		}

		prev = current;
		current = current->next;
	} while (current != NULL);

	if (current != NULL) {
		ret = current->data;
	}

	/* if the first element */
	if (current == *head) {
		/* reuse prev */
		prev = *head;
		*head = current->next;
		free(prev);
		return ret;
	}

	/* if the last element */
	if (current->next == NULL) {
		prev->next = NULL;
		free(current);
		return ret;
	}

	prev->next = current->next;
	free(current);

	return ret;
}

void print_list(TList* head) {
	TList* current = head;
	
	printf("[ ");
	while (current != NULL) {
		printf("%d ", current->seq_no);
		current = current->next;
	}

	printf("]\n");
}

void destroy_list(TList** head) {
	TList* node = *head;
	do {
		TList* tmp;
		tmp = node;
		node = node->next;
		free(tmp);
	} while (node != NULL);
}