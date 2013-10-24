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

typedef struct scheduler {
	ucontext_t* context;

	/* Tasks ready to execute */
	volatile list_t ready;
	/* Tasks waiting on something, blocked tasks.*/
	list_t waiting;

	int top;

	task_t* new_task;
	task_t* current_task;

	sched_action_t action;
} scheduler_t;

void sched_init();
void sched_execute_action(void *arguments);
void sched_add_task(task_t* new_task);
void sched_yield_current();
void sched_invoke();
void sched_wrapper_function(void* c);
scheduler_t* sched_get();

void sched_print_list(list_t list);

scheduler_t scheduler;

static int task_counter = 1;

void sched_init() {
	scheduler.context = (ucontext_t*) malloc(sizeof(ucontext_t));
	//allocate the stack for the ucontext
	ALLOCATE_STACK_UC(scheduler.context);

	scheduler.ready = INIT_LIST();
	scheduler.waiting = INIT_LIST();

	//allocate the scheduler function
	getcontext(scheduler.context);
	makecontext(scheduler.context, (void (*) (void)) sched_execute_action, 0);
}

/**
 * This is called after a new task is spawned. What we want to do is to place the currently
 * running task (the parent) into the queue and run the child.
 */
void sched_execute_action(void *arguments) {
	getcontext(scheduler.context);
	if (scheduler.action == ADD_TASK) {
		list_push_back(scheduler.ready,  scheduler.current_task);
		debug("New task %d to be executed, %d placed on queue", scheduler.new_task->id, task_current()->id);
		scheduler.current_task = scheduler.new_task;
	} else if (scheduler.action == YIELD) {
		list_push_back(scheduler.ready, scheduler.current_task);
		task_t* new_task = (task_t*) list_pop_head(scheduler.ready);
		debug("Task %d yielding, task %d executing", task_current()->id, new_task->id);
		scheduler.current_task = new_task;
	} else if (scheduler.action == RETURN_TASK) {
		scheduler.current_task->status = COMPLETED;
		task_t* new_task = (task_t*) list_pop_head(scheduler.ready);
		debug("Task %d returned %d, task %d executing" ,task_current()->id, *((int*)scheduler.current_task->result), new_task->id);
		scheduler.current_task = new_task;

	}
	//sched_print_list(scheduler.ready); this might cause a buffer overflow
	setcontext(task_current()->context);

}

void sched_add_task(task_t* new_task) {
	scheduler.new_task = new_task;
	scheduler.action = ADD_TASK;
	sched_invoke();
}

void sched_yield_current() {
	scheduler.action = YIELD;
	sched_invoke();
}

void sched_invoke() {
	//debug("Task %d invoking scheduler...\n", task_current()->id);
	swapcontext(task_current()->context, scheduler.context);
	//debug("Task %d returning from invoke..\n", task_current()->id);
}

/**
 * Initialize the necessary tasks
 */
void task_init() {
	sched_init();
}

/**
 * Perform cleanup.
 */
void task_deinit() {
	free(sched_get()->context);
	free(sched_get()->current_task);
	free(sched_get()->new_task);
}

/**
 * Create a new execution context and create a pointer to it.
 */
task_t* task_spawn(void* fct_ptr, void* arguments, void *return_val) {
	task_t* new_task;
	ALLOCATE_TASK(new_task);

	new_task->arguments = (void*) arguments;
	new_task->function = fct_ptr;
	new_task->result = return_val;
	new_task->status = STARTED;
	new_task->context->uc_link = sched_get()->context;

	getcontext(new_task->context);
	if (new_task->status == STARTED) {
		makecontext(new_task->context, (void (*) (void)) sched_wrapper_function, 0);
		sched_add_task(new_task);
	}
	return new_task;
}

int task_return() {
	scheduler.action = RETURN_TASK;
	sched_invoke();
	return 0;
}

void sched_wrapper_function(void* c) {
	task_t* context = scheduler.current_task;
	context->function(context->arguments);
}

scheduler_t* sched_get() {
	return &scheduler;
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
