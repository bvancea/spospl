#ifndef ALLOC_H_
#define ALLOC_H_

#include "task.h"

#define ALLOCATE_TASK(task, counter, lock)		task = (task_t*) malloc(sizeof(task_t));    \
								task->context = (ucontext_t*) malloc(sizeof(ucontext_t)); 	\
								ALLOCATE_STACK_UC(task->context);							\
								task->children = 0;											\
								pthread_mutex_lock(&lock);									\
								task->id = ++counter;                                  \
								pthread_mutex_unlock(&lock)	

#define ALLOCATE_FIRST(task )   task = (task_t*) malloc(sizeof(task_t));					\
								task->context = (ucontext_t*) malloc(sizeof(ucontext_t)); 	\
								ALLOCATE_STACK_UC(task->context);							\
								task->id = 1

#define ALLOCATE_STACK_UC(context)	ALLOCATE_STACK(context->uc_stack)
#define ALLOCATE_STACK(stack) 	stack.ss_sp = malloc(STACK_SIZE); \
								stack.ss_size = STACK_SIZE; \
								stack.ss_flags = 0
#define FREE_STACK_UC(context)  FREE_STACK(context.uc_stack)
#define FREE_STACK(stack)		free(stack.ss_sp)
#define LINK(child, parent)		child.uc_link = &parent;
#define SAFE_FREE(pointer)		if (pointer != NULL) { \
									free(pointer);	   \
								}  					\
								pointer = NULL


task_t* task_malloc();
void task_free(task_t* );
task_t* task_malloc_new();
#endif /* ALLOC_H_ */