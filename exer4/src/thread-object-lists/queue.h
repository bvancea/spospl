#ifndef QUEUE_H_
#define QUEUE_H_

#include "task.h"

void queue_push(queue_t* queue, task_t* task);
void queue_push_if_not_contained(queue_t* queue, task_t* task);
task_t* queue_pop(queue_t* queue);
int queue_contains(queue_t* queue, task_t* task);
void print_queue(queue_t* queue);
void queue_free(queue_t* queue);

#endif /* QUEUE_H_ */