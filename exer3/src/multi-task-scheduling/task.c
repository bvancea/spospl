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
#include <math.h>
#include <time.h>
#include <sched.h>
#include "task.h"
#include "lists.h"
#include "dbg.h"

#define STACK_SIZE	64 * 1024

#define ALLOCATE_TASK(task)		task = (task_t*) malloc(sizeof(task_t));					\
								task->context = (ucontext_t*) malloc(sizeof(ucontext_t)); 	\
								ALLOCATE_STACK_UC(task->context);							\
								task->children = 0;															\
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
#define SAFE_FREE(pointer)		if (pointer != NULL) { \
									free(pointer);	   \
								}  					\
								pointer = NULL

pthread_cond_t wake_up_threads = PTHREAD_COND_INITIALIZER, wake_up_main = PTHREAD_COND_INITIALIZER;
pthread_mutex_t wake_up_threads_lock = PTHREAD_MUTEX_INITIALIZER, wake_up_main_lock = PTHREAD_MUTEX_INITIALIZER;

ucontext_t go_home;

void queue_push(queue_t* queue, task_t* task) {
	pthread_mutex_lock(&queue->lock);
	queue->tasks[++queue->top] = task;
	pthread_mutex_unlock(&queue->lock);
}

task_t* queue_pop(queue_t* queue) {
	task_t* returned;

	pthread_mutex_lock(&queue->lock);
	if (queue->top == 0) {
		returned = NULL; 
	} else {
		returned = queue->tasks[queue->top--];	
	}

	pthread_mutex_unlock(&queue->lock);
	return returned;
}

int queue_contains(queue_t* queue, task_t* task) {
	pthread_mutex_lock(&queue->lock);
	int size = queue->top;
	int contained = 0;
	while (size > 0) {
		if ( queue->tasks[size--]->id == task->id) {
			contained = 1;
		}
	}
	pthread_mutex_unlock(&queue->lock);
	return contained;
}

void print_queue(queue_t* queue) {
#ifndef NDEBUG
	pthread_mutex_lock(&queue->lock);
	int size = queue->top;
	while (size > 0) {
		printf("%d ", queue->tasks[size--]->id);
	}
	printf("\n");
	pthread_mutex_unlock(&queue->lock);
#endif
}

void queue_free(queue_t* queue) {
	pthread_mutex_lock(&queue->lock);
	int size = 63;
	while (size > 0) {
		SAFE_FREE(queue->tasks[size--]);
	}
	printf("\n");
	pthread_mutex_unlock(&queue->lock);	
	pthread_mutex_destroy(&queue->lock);
}

scheduler_t* schedulers;
int schedulers_nr;
static int task_counter = 1;

pthread_key_t scheduler_key;
pthread_attr_t attr;
program_t program;

void* thread_init(void* arg) {

	scheduler_t* local_scheduler = (scheduler_t *) arg;
	if (local_scheduler == NULL) {
		debug("Local scheduler seems NULL");
	}
	pthread_setspecific(scheduler_key, (void*) &local_scheduler->id);

	sched_execute_action(NULL);
	return (void*) 0xbeef;
}

/*
 *	Initialize the thread scheduler context. There are n scheduler contexts, one for each thread.
 */
void sched_init(int workers) {

	debug("Initializing workers");
	pthread_key_create(&scheduler_key, task_deinit);
	schedulers = (scheduler_t *) malloc(workers * sizeof(scheduler_t));
	
	int i;
	for (i = 0; i < workers; ++i) {
		debug("Creating thread %d", i);
		schedulers[i].context = (ucontext_t*) malloc(sizeof(ucontext_t));
		//allocate the stack for the ucontext
		ALLOCATE_STACK_UC(schedulers[i].context);

		//schedulers[i].ready = INIT_LIST();
		schedulers[i].action = IDLE;
		schedulers[i].id = i;
		//allocate the scheduler function
		schedulers[i].ready.top = 0;
		schedulers[i].new_task = NULL;
		schedulers[i].current_task = NULL;
		pthread_mutex_init(&schedulers[i].lock, NULL);
		pthread_mutex_init(&schedulers[i].ready.lock, NULL);
		getcontext(schedulers[i].context);
		makecontext(schedulers[i].context, (void (*) (void)) sched_execute_action, 0);		
	}	

	for (i = 0; i < workers; ++i) {
		pthread_create(&schedulers[i].thread, NULL, thread_init, (void*) &schedulers[i]);
	}

	getcontext(&go_home);

	if (!inside_main()) {		
		scheduler_t* scheduler = sched_get(); 
		debug("%d - Jumping to scheduler context", scheduler->id);		
		setcontext(scheduler->context);
	} else {
		debug("Only main here");
	}
}

/**
 * This is called after a new task is spawned. What we want to do is to place the currently
 * running task (the parent) into the queue and run the child.
 */
void* sched_execute_action(void *arguments) {	
	debug("Inside scheduler");
	if (inside_main()) {
		log_err("Main inside a scheduler context. Exiting...");
		exit(-1);
	}
	scheduler_t* scheduler = sched_get();	
	int cpu_core = scheduler->id % sysconf(_SC_NPROCESSORS_ONLN);
	cpu_set_t cpuset;
   	CPU_ZERO(&cpuset);
   	CPU_SET(cpu_core, &cpuset);
   	pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
	getcontext(scheduler->context);
		
	if (sched_get()->context != scheduler->context) {
		debug("%d - In the wrong stack %d", sched_get()->id, scheduler->id);
		setcontext(sched_get()->context);
	} else {
		debug("%d - In the right stack ", scheduler->id);
	}

	pthread_mutex_lock(&scheduler->lock);

	if (has_program_ended()) {
		free(scheduler->context->uc_stack.ss_sp);
		free(scheduler->context);
		debug("%d - Program ended, attempting to exit",scheduler->id);
		pthread_mutex_unlock(&scheduler->lock);
		pthread_exit(0);
	}

	if (scheduler->action == ADD_TASK) {
		sched_handler_add_task(scheduler);		
	} else if (scheduler->action == YIELD) {
		sched_handler_yield(scheduler);
	} else if (scheduler->action == RETURN_TASK) {
		sched_handler_return(scheduler);				
	} else if (scheduler->action == IDLE) {
		
		sched_handler_try_steal(scheduler);	
		//failed to steal
		// if (scheduler->action == IDLE) {
		// 	// pthread_mutex_unlock(&scheduler->lock);
		// 	// pthread_mutex_lock(&wake_up_threads_lock);
		// 	// pthread_cond_wait(&wake_up_threads, &wake_up_threads_lock);
		// 	// pthread_mutex_unlock(&wake_up_threads_lock);

		// 	// pthread_mutex_lock(&scheduler->lock);
		// }
	} 
	ucontext_t* next_context;
	if (scheduler->current_task != NULL) {
		debug("%d - Attempting to execute task %d", scheduler->id, scheduler->current_task->id);
		next_context = scheduler->current_task->context;
	} else {
		next_context = scheduler->context;
	}
	pthread_mutex_unlock(&scheduler->lock);
	setcontext(next_context);

	return (void*) 0xbeef;
}

void sched_handler_add_task(scheduler_t* scheduler) {
	if (scheduler->current_task != NULL) {
		debug("%d - New task %d to be executed, %d added to queue...",scheduler->id, scheduler->new_task->id, scheduler->current_task->id);
		if (!queue_contains(&scheduler->ready, scheduler->current_task)) {
			queue_push(&scheduler->ready, scheduler->current_task);
		}
	} else {
		debug("%d - New task %d to be executed, called from main.", scheduler->id, scheduler->new_task->id);
	}

	scheduler->current_task = scheduler->new_task;
	scheduler->new_task = NULL;
	scheduler->action = YIELD;
}

void sched_handler_yield(scheduler_t* scheduler) {
	task_t* new_task = (task_t*) queue_pop(&scheduler->ready);
	if (new_task != NULL && new_task->context != NULL) {		
		scheduler->current_task = new_task;			
	} else {		
		scheduler->action = IDLE;
		scheduler->current_task = NULL;
	}
}

void sched_handler_return(scheduler_t* scheduler) {
	//pthread_mutex_lock(&scheduler->lock);
	debug("%d - Task %d returned %d", scheduler->id, scheduler->current_task->id, *((int*) (scheduler->current_task->result)));
	scheduler->current_task->status = COMPLETED;
	if (scheduler->current_task->parent) {
		task_dec_children_count(scheduler->current_task->parent);
		if (scheduler->current_task->parent->children == 0 && scheduler->current_task->parent->status != COMPLETED) {
			debug("%d - Task %d added back to queue.",scheduler->id, scheduler->current_task->parent->id);
			if (!queue_contains(&scheduler->ready, scheduler->current_task)) {
				queue_push(&scheduler->ready, scheduler->current_task->parent);
			}
		}
	} else {
		pthread_mutex_lock(&wake_up_main_lock);
		pthread_cond_signal(&wake_up_main);
		pthread_mutex_unlock(&wake_up_main_lock);
	}

	scheduler->action = YIELD;
	scheduler->current_task = NULL;
}

void sched_handler_try_steal(scheduler_t* scheduler) {
	if (schedulers_nr > 1) {
		unsigned int seed = 42;
		srand(time(NULL));
		long victim_id;
	
		do {
			victim_id = rand_r(&seed) % schedulers_nr;
		} while (victim_id == scheduler->id);

		scheduler_t* victim = &schedulers[victim_id];
		task_t* stolen = queue_pop(&victim->ready);
		debug("%d - Queue of the victim", scheduler->id);
		print_queue(&victim->ready);
		debug("%d - My queue", scheduler->id);
		print_queue(&scheduler->ready);
		if (stolen ) {
			debug("%d - Succesfully stolen task %d from victim %d", scheduler->id, stolen->id, (int) victim_id);
			//stolen->context->uc_link = scheduler->context;
			scheduler->action = YIELD;
			scheduler->current_task = stolen;
		} else {
			debug("%d - Failed to steal from victim %d", scheduler->id, (int) victim_id);
		}		
	}
}

void sched_add_task(task_t* new_task) {
	scheduler_t* scheduler = sched_get();

retry:
	pthread_mutex_lock(&scheduler->lock);
	if (scheduler->new_task != NULL) {
		pthread_mutex_unlock(&scheduler->lock);
		goto retry;
	}
	scheduler->new_task = new_task;
	scheduler->action = ADD_TASK;
	// pthread_mutex_lock(&wake_up_threads_lock);
	// debug("Main broadcasting to %d", scheduler->id);
	// pthread_cond_broadcast(&wake_up_threads);
	// pthread_mutex_unlock(&wake_up_threads_lock);

	pthread_mutex_unlock(&scheduler->lock);	
	if (!inside_main()) {
		sched_invoke();	
	}	
}

void sched_yield_current() {
	sched_get()->action = YIELD;
	sched_invoke();
}

void sched_invoke() {
	scheduler_t *scheduler = sched_get();
	if (scheduler->current_task) {
		swapcontext(scheduler->current_task->context, scheduler->context);
	} else {
		setcontext(scheduler->context);
	}
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
	int* id = (int *) pthread_getspecific(scheduler_key);
	scheduler_t* scheduler;
	if (id == NULL) {
		debug("Returning default scheduler.");
		unsigned int seed = 1;
		srand(time(NULL));
		int index = rand_r(&seed) % schedulers_nr;
		scheduler = &schedulers[index];
	} else {
		scheduler = &schedulers[*id];
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
	schedulers_nr = workers;
	start_program();
	sched_init(workers);
}

/**
 * Perform cleanup.
 */
void task_deinit(void * arg) {
	debug("Thread exiting...");		
}

void task_end() {
	debug("Main waiting for pthreads to arrive here");
	int i;
	end_program();
	debug("End in pthread %d", (int) pthread_self());

	for (i = 0; i < schedulers_nr; ++i) {
		debug("Waiting for thread %d", i);
		//pthread_cond_broadcast(&wake_up_threads);
		pthread_join(schedulers[i].thread, NULL);	
	}

	for (i = 0; i < schedulers_nr; ++i) {		
		pthread_mutex_destroy(&schedulers[i].ready.lock);
		pthread_mutex_destroy(&schedulers[i].lock);
	}
	
	SAFE_FREE(schedulers);
	debug("End reached");
}

/**
 * Create a new execution context and create a pointer to it.
 */
task_t* task_spawn(void* fct_ptr, void* arguments, void *return_val) {
	task_t* new_task;
	task_t* current_task = task_current();
	ALLOCATE_TASK(new_task);
	pthread_mutex_init(&new_task->lock, NULL);
	if (inside_main()) {
		debug("Spawn called from main.");
	} else {
		int parent_id = (current_task->parent) ? (current_task->parent->id) : 0;
		if (parent_id < 0 ) exit(1);
		debug("Spawn called from a worker %d, parent %d", current_task->id, parent_id);
	}
	*((int*)return_val) = -1;
	new_task->arguments = (void*) arguments;
	new_task->function = (function_t) fct_ptr;
	new_task->result = return_val;
	new_task->status = STARTED;
	new_task->context->uc_link = &go_home;
	new_task->parent = current_task;

	if (!inside_main()) {
		task_inc_children_count(current_task);	
	}		
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
	task->children -= 1;
	pthread_mutex_unlock(&lock);	
}


task_t* task_current() {
	if (inside_main()) {
		return 0;
	} else {
		scheduler_t* scheduler = sched_get();
		return scheduler->current_task;
	}
}

int has_program_ended() {
	if (program.state == PROGRAM_ENDED) {
		return 1;
	} else {
		return 0;
	}
}

void start_program() {
	pthread_mutex_lock(&program.lock);
	program.state = PROGRAM_STARTED;
	pthread_mutex_unlock(&program.lock);
}

void end_program() {
	pthread_mutex_lock(&program.lock);
	program.state = PROGRAM_ENDED;
	pthread_mutex_unlock(&program.lock);
}

int inside_main() {
	if (pthread_getspecific(scheduler_key) == NULL) {
		return 1;
	} else {
		return 0;
	}
}

int task_sync(task_t** execution_context, int count) {

	int i;
	int someone_not_done = 1;
	while (someone_not_done) {
		
		someone_not_done = 0;
		for (i = 0; i < count; i++) {
			if (execution_context[i]->status != COMPLETED) {
			//if (execution_context[i]->status != COMPLETED || execution_context[i]->result == NULL || *((int*)execution_context[i]->result) == -1) {
				someone_not_done = 1;
				if (!inside_main()) {
					debug("%d - Yielding...", sched_get()->id);
					sched_yield_current();
				} else {
					pthread_mutex_lock(&wake_up_main_lock);
					pthread_cond_wait(&wake_up_main, &wake_up_main_lock);
					pthread_mutex_unlock(&wake_up_main_lock);
				}
			}
		}
		
	}

	//if control is here, child tasks have finished
	//let's clean-up
	
	for (i = 0; i < count; i++) {
		task_t* task = execution_context[i];
		task_destroy(task);
	}

	if (inside_main()) {
		debug("Going outside of sync in main");
	} else {
		debug("Going outside of sync in task %d", task_current()->id);
	}
	
	return 0;
}

void task_destroy(task_t* task) {
	if (task) {
		SAFE_FREE(task->context->uc_stack.ss_sp);
		SAFE_FREE(task->context);
		SAFE_FREE(task);	
	}
}

int task_first_child(task_t* task) {
	return -1;
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

