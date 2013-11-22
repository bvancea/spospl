/*
 * lists.h
 *
 *	Double linked list API implementation
 *
 *  Created on: Oct 23, 2013
 *      Author: bogdan
 */
#include "lists.h"
#include "dbg.h"
#include <stdlib.h>

node_t safe_list_head(list_t list) {
	pthread_mutex_lock(&list->lock);

	node_t returned;

	if (list->head == NULL) {
		returned = NULL;
	} else {
		returned = list->head;
	}

	pthread_mutex_unlock(&list->lock);
	return returned;
}

void* list_pop_tail(list_t list) {
	void* returned;
	pthread_mutex_lock(&list->lock);

	if (list->head == list->tail) {
		returned = NULL;
	} else {
		returned = list->tail->content;
		node_t deleted = list->tail;

		list->tail = list->tail->prev;
		if (list->tail != NULL) {
			list->tail->next = NULL;
		} else {
			list->head = NULL;
		}
		free(deleted);
	}

	pthread_mutex_unlock(&list->lock);

	return returned;
}

void* list_pop_head(list_t list) {

	pthread_mutex_lock(&list->lock);

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
		if (deleted == NULL) {
			log_err("Deleted is NULL");
		}
		//free(deleted);
	}
	pthread_mutex_unlock(&list->lock);
	return returned;
}

list_t list_push_back(list_t list, void* payload) {

	pthread_mutex_lock(&list->lock);
	
	node_t node = (node_t) malloc(sizeof(*node));
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

	pthread_mutex_unlock(&list->lock);
	return list;
}


