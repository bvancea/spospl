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
#include <pthread.h>

typedef struct element {
	void* content;
	struct element *next;
	struct element *prev;
} node;

typedef node* node_t;

typedef struct list {
	node_t head;
	node_t tail;	
	pthread_mutex_t lock;
}* list_t;

list_t list_push_back(list_t list, void* payload);
node_t list_head(list_t list);
void* list_pop_head(list_t list);
void* list_pop_tail(list_t list);
list_t list_safe_push_back(list_t list, void* payload);
node_t list_safe_head(list_t list);
void* list_safe_pop_head(list_t list);
void* list_safe_pop_tail(list_t list);


void print_list(list_t list);



#endif /* LISTS_H_ */
