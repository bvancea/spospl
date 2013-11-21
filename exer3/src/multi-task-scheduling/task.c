/*
 * task.c
 *
 *	Implementation for the task API.
 *  Created on: Oct 22, 2013
 *      Author: bogdan
 */
#include <ucontext.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include "task.h"
#include "lists.h"
#include "dbg.h"

#define STACK_SIZE	4 * 1028

#define ALLOCATE_TASK(task)		task = (task_t*) malloc(sizeof(task_t));					\
								task->context = (ucontext_t*) malloc(sizeof(ucontext_t)); 	\
								ALLOCATE_STACK_UC(task->context);							\
								task->id = ++task_counter
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

#define QUEUE_SIZE 100


scheduler_t* schedulers;
static int task_counter = 1;
pthread_key_t scheduler_key;

void* thread_init(void* arg) {

	scheduler_t* local_scheduler = (scheduler_t *) arg;
	pthread_setspecific(scheduler_key, (void*) local_scheduler);
	setcontext(local_scheduler->context);

	return (void*) 0xbeef;
}

/*
 *	Initialize the thread scheduler context. There are n scheduler contexts, one for each thread.
 */
void sched_init(int workers) {

	pthread_key_create(&scheduler_key, task_deinit);
	schedulers = (scheduler_t *) malloc(workers * sizeof(scheduler_t));

	for (int i = 0; i < workers; ++i) {
		schedulers[i].context = (ucontext_t*) malloc(sizeof(ucontext_t));
		//allocate the stack for the ucontext
		ALLOCATE_STACK_UC(schedulers[i].context);

		schedulers[i].ready = INIT_LIST();
		schedulers[i].waiting = INIT_LIST();

		//allocate the scheduler function
		getcontext(schedulers[i].context);
		makecontext(schedulers[i].context, (void (*) (void)) sched_execute_action, 0);
		pthread_create(&schedulers[i].thread, NULL, sched_execute_action, NULL);
	}	
}

/**
 * This is called after a new task is spawned. What we want to do is to place the currently
 * running task (the parent) into the queue and run the child.
 */
void* sched_execute_action(void *arguments) {	
	scheduler_t* scheduler = sched_get();
	getcontext(scheduler->context);

	if (scheduler->action == ADD_TASK) {
		list_push_back(scheduler->ready,  scheduler->current_task);
		debug("New task %d to be executed, %d placed on queue", scheduler->new_task->id, task_current()->id);
		scheduler->current_task = scheduler->new_task;
	} else if (scheduler->action == YIELD) {
		list_push_back(scheduler->ready, scheduler->current_task);
		task_t* new_task = (task_t*) list_pop_head(scheduler->ready);
		debug("Task %d yielding, task %d executing", task_current()->id, new_task->id);
		scheduler->current_task = new_task;
	} else if (scheduler->action == RETURN_TASK) {
		scheduler->current_task->status = COMPLETED;
		task_t* new_task = (task_t*) list_pop_head(scheduler->ready);
		debug("Task %d returned %d, task %d executing" ,task_current()->id, *((int*)scheduler->current_task->result), new_task->id);
		scheduler->current_task = new_task;
	}
	
	//sched_print_list(scheduler.ready); this might cause a buffer overflow
	setcontext(task_current()->context);

	return (void*) 0xbeef;
}

void sched_add_task(task_t* new_task) {
	scheduler_t* scheduler = sched_get();
	scheduler->new_task = new_task;
	scheduler->action = ADD_TASK;
	sched_invoke();
}

void sched_yield_current() {
	sched_get()->action = YIELD;
	sched_invoke();
}

void sched_invoke() {
	swapcontext(task_current()->context, sched_get()->context);
}

void sched_wrapper_function(void* c) {
	task_t* context = sched_get()->current_task;
	context->function(context->arguments);
}

/*
 *	Returns a pointer to the schedular structure of the the local thread. If this
 *  function is called outside of the threads 
 */
scheduler_t* sched_get() {
	scheduler_t* scheduler = (scheduler_t*) pthread_getspecific(scheduler_key);
	if (scheduler == NULL) {
		scheduler = &schedulers[0];	
	}
	return scheduler;
}

void sched_print_list(list_t list) {
	node_t current;
	char message[1024];
	strcpy(message, "Scheduling queue is: ");
	for (current = list->head; current != NULL; current = current->next) {
		if (current->content) {
			int payload = ((task_t *) current->content)->id;
			char current_elem[10];
			sprintf(current_elem, "%d ", payload);
			strcat(message, current_elem);
		} else {
			log_err("Scheduling list contains NULL elements.Exiting...");
			exit(0);
		}
	}
	debug("%s",message);
}

/**
 * Initialize the necessary tasks
 */
void task_init(int workers) {

	sched_init(workers);
}

/**
 * Perform cleanup.
 */
void task_deinit(void * arg) {
	scheduler_t* local_scheduler = sched_get();	
	free(local_scheduler->context);
	free(local_scheduler->current_task);
	free(local_scheduler->new_task);
}

/**
 * Create a new execution context and create a pointer to it.
 */
task_t* task_spawn(void* fct_ptr, void* arguments, void *return_val) {
	task_t* new_task;
	ALLOCATE_TASK(new_task);

	new_task->arguments = (void*) arguments;
	new_task->function = (function_t) fct_ptr;
	new_task->result = return_val;
	new_task->status = STARTED;
	new_task->context->uc_link = sched_get()->context;
	new_task->parent = task_current();

	getcontext(new_task->context);
	if (new_task->status == STARTED) {
		makecontext(new_task->context, (void (*) (void)) sched_wrapper_function, 0);
		sched_add_task(new_task);
	}
	return new_task;
}

int task_return() {
	sched_get()->action = RETURN_TASK;
	sched_invoke();
	return 0;
}

void task_inc_children_count(task_t* task) {
	pthread_mutex_t lock = task->lock;
	pthread_mutex_lock(&lock);
	task->children += 1;
	pthread_mutex_unlock(&lock);
}

void task_dec_children_count(task_t* task) {
	pthread_mutex_t lock = task->lock;
	pthread_mutex_lock(&lock);
	task->children += 1;
	pthread_mutex_unlock(&lock);	
}


task_t* task_current() {
	scheduler_t* scheduler = sched_get();
	if (!scheduler->current_task) {
		ALLOCATE_FIRST(scheduler->current_task);
	}
	return scheduler->current_task;
}

int task_sync(task_t** execution_context, int count) {

	int i;
	int someone_not_done = 1;
	while (someone_not_done) {
		someone_not_done = 0;
		for (i = 0; i < count; i++) {
			if (execution_context[i]->status != COMPLETED) {
				debug("Task %d not done, %d yielding", execution_context[i]->id, task_current()->id);
				someone_not_done = 1;
				sched_yield_current();
			}
		}
	}

	//if control is here, child tasks have finished
	//let's clean-up
	for (i = 0; i < count; i++) {
		task_t* task = execution_context[i];
		free(task->context);
		free(task);
	}
	return 0;
}

/*
 *	Main loop of the worker threads. 
 *	
 *	Worker threads either execute tasks from their own queues or try to steal tasks
 *  from the stacks of other schedulers.
 */
void* worker_loop() {

	return NULL;
}
