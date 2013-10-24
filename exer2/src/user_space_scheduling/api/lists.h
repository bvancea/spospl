/*
 * lists.h
 *
 *	This header exposes some a double linked list API
 *
 *  Created on: Oct 23, 2013
 *      Author: bogdan
 */

#ifndef LISTS_H_
#define LISTS_H_
#include <stdio.h>

typedef struct element {
	void* content;
	struct element *next;
	struct element *prev;
}node;

typedef node* node_t;

typedef struct list {
	node_t head;
	node_t tail;
}* list_t;

list_t list_push_back(list_t list, void* payload);
node_t list_head(list_t list);
void* list_pop_head(list_t list);
void print_list(list_t list);

#define INIT_LIST() (list_t) malloc(sizeof(list_t));


#endif /* LISTS_H_ */
