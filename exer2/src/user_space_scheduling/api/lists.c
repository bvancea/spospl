/*
 * lists.h
 *
 *	Double linked list API implementation
 *
 *  Created on: Oct 23, 2013
 *      Author: bogdan
 */
#include "lists.h"
#include <stdlib.h>

node_t list_head(list_t list) {
	node_t returned;
	if (list->head == NULL) {
		returned = NULL;
	} else {
		returned = list->head;
	}
	return returned;
}

void* list_pop_head(list_t list) {
	void* returned;
	if (list->head == NULL) {
		returned = NULL;
	} else {
		returned = list->head->content;
		node_t deleted = list->head;
		list->head = list->head->next;
		if (list->head != NULL) {
			list->head->prev = NULL;
		} else {
			list->tail = NULL;
		}
		free(deleted);
	}
	return returned;
}

list_t list_push_back(list_t list, void* payload) {
	node_t node = (node_t) malloc(sizeof(node_t));
	node->content = payload;
	if (list->head == NULL && list->tail == NULL) {
		list->head = node;
		list->tail = node;
	} else {
		list->tail->next = node;					//connect tail to new node
		node->prev = list->tail;					//connect new node to current tail
		node->next = NULL;							//nowhere to go next
		list->tail = node;							//we have a new tail
	}
	return list;
}


