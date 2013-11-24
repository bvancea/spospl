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
#include "dbg.h"

#define STACK_SIZE	256 * 1024

#define ALLOCATE_TASK(task)		task = (task_t*) malloc(sizeof(task_t));					\
								task->context = (ucontext_t*) malloc(sizeof(ucontext_t)); 	\
								ALLOCATE_STACK_UC(task->context);							\
								task->children = 0;											\
								pthread_mutex_lock(&program.lock);							\
								task->id = ++task_counter;                                   \
								pthread_mutex_unlock(&program.lock)							
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

static ucontext_t go_home;
static unsigned int main_id = -1;

void queue_push(queue_t* queue, task_t* task) {
	pthread_mutex_lock(&queue->lock);
	queue->tasks[++queue->top] = task;
	pthread_mutex_unlock(&queue->lock);
}

void queue_push_if_not_contained(queue_t* queue, task_t* task) {
	pthread_mutex_lock(&queue->lock);
	int size = queue->top;
	int contained = 0;
	while (size > 0) {
		if ( queue->tasks[size--]->id == task->id) {
			contained = 1;
		}
	}
	if (!contained) {
		queue->tasks[++queue->top] = task;	
	}
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

static scheduler_t* schedulers;
static int schedulers_nr;
static volatile int task_counter = 1;

static pthread_key_t scheduler_key;
pthread_attr_t attr;
static program_t program;

void* thread_init(void* arg) {

	scheduler_t* local_scheduler = (scheduler_t *) arg;
	if (local_scheduler == NULL) {
		debug("Local scheduler seems NULL");
	}
	debug("Called in pthread: %d", (unsigned int) pthread_self());
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
	int main_id = -1;
	pthread_setspecific(scheduler_key,(void*) &main_id);
	for (i = 0; i < workers; ++i) {
		pthread_create(&schedulers[i].thread, NULL, thread_init, (void*) &schedulers[i]);
	}

	getcontext(&go_home);

	if (!inside_main()) {		
		scheduler_t* scheduler = sched_get(); 
		if (!scheduler) {
			log_err("NO SCHEDULER");
		}
		debug("%d - Jumping to scheduler context", scheduler->id);		
		setcontext(scheduler->context);
	} else {
		debug("Only main here");
	}
}

void sched_assign_to_core(scheduler_t* scheduler, int cores_number) {
	int cpu_core = scheduler->id % cores_number;
	cpu_set_t cpuset;
   	CPU_ZERO(&cpuset);
   	CPU_SET(cpu_core, &cpuset);
   	pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
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
	//sched_assign_to_core(scheduler, sysconf(_SC_NPROCESSORS_ONLN));
	getcontext(scheduler->context);
		
	if (has_program_ended()) {
		free(scheduler->context->uc_stack.ss_sp);
		free(scheduler->context);
		debug("%d - Program ended, attempting to exit",scheduler->id);
		pthread_exit(0);
	}

	if (scheduler->action == ADD_TASK) {
		sched_handler_add_task(scheduler);		
	} else if (scheduler->action == YIELD) {
		sched_handler_yield(scheduler);
	} else if (scheduler->action == RETURN_TASK) {
		sched_handler_return(scheduler);				
	} else {
		scheduler->current_task = NULL;
		task_t* new_task = (task_t*) queue_pop(&scheduler->ready);
		if (new_task) {
			scheduler->current_task = new_task;
		} else {
			sched_handler_try_steal(scheduler, 3);		
		}				
	} 
	ucontext_t* next_context = scheduler->context;

	if (scheduler->current_task) {
		pthread_mutex_lock(&scheduler->current_task->running_lock);
		debug("%d - Attempting to execute task %d", scheduler->id, scheduler->current_task->id);		
		if (scheduler->current_task->status != COMPLETED) {
			next_context = scheduler->current_task->context;
		} else {
			pthread_mutex_unlock(&scheduler->current_task->running_lock);	
		}
	} 
	
	setcontext(next_context);

	return (void*) 0xbeef;
}

void sched_handler_add_task(scheduler_t* scheduler) {
	if (scheduler->current_task != NULL) {
		debug("%d - New task %d to be executed, %d added to queue...",scheduler->id, scheduler->new_task->id, scheduler->current_task->id);
		pthread_mutex_unlock(&scheduler->current_task->running_lock);
		queue_push_if_not_contained(&scheduler->ready, scheduler->current_task);
	} else {
		debug("%d - New task %d to be executed, called from main.", scheduler->id, scheduler->new_task->id);
	}

	scheduler->current_task = scheduler->new_task;
	scheduler->new_task = NULL;
	scheduler->action = YIELD;
}

void sched_handler_yield(scheduler_t* scheduler) {
	
	if (scheduler->current_task) pthread_mutex_unlock(&scheduler->current_task->running_lock);
	
	task_t* new_task = (task_t*) queue_pop(&scheduler->ready);
	if (new_task != NULL && new_task->context != NULL) {		
		scheduler->current_task = new_task;			
	} else {		
		scheduler->action = IDLE;
		scheduler->current_task = NULL;
	}
}

void sched_handler_return(scheduler_t* scheduler) {
	debug("%d - Task %d returned %d", scheduler->id, scheduler->current_task->id, *((int*) (scheduler->current_task->result)));
	
	scheduler->current_task->status = COMPLETED;

	pthread_mutex_unlock(&scheduler->current_task->running_lock);

	if (scheduler->current_task->parent) {
		task_dec_children_count(scheduler->current_task->parent);
		if (scheduler->current_task->parent->children == 0 && scheduler->current_task->parent->status != COMPLETED) {
			debug("%d - Task %d added back to queue.",scheduler->id, scheduler->current_task->parent->id);
			queue_push_if_not_contained(&scheduler->ready, scheduler->current_task->parent);
		}
	} else {
		//wake up the main thread
		debug("Waking up main.");
		pthread_mutex_lock(&wake_up_main_lock);
		pthread_cond_signal(&wake_up_main);
		pthread_mutex_unlock(&wake_up_main_lock);
	}

	scheduler->action = IDLE;
	scheduler->current_task = NULL;
}

void sched_handler_try_steal(scheduler_t* scheduler, int steal_attempts) {
	if (schedulers_nr > 1) {
		unsigned int seed = 42;
		srand(time(NULL));
		long victim_id = 0;
		int attempt = 0;
		task_t* stolen = NULL;

		while (stolen == NULL && attempt < steal_attempts) {
			do {
				victim_id = rand_r(&seed) % schedulers_nr;
			} while (victim_id == scheduler->id);

			scheduler_t* victim = &schedulers[victim_id];
			stolen = queue_pop(&victim->ready);
			if (stolen) {
				debug("%d - yay", scheduler->id);
			}
			attempt+=1;
		}
		if (stolen) {
			debug("%d - Succesfully stolen task %d from victim %d", scheduler->id, stolen->id, (int) victim_id);
			scheduler->action = YIELD;
			scheduler->current_task = stolen;
		} else {
			//debug("%d - Failed to steal from victim %d", scheduler->id, (int) victim_id);
		}		
	}
}

void sched_add_task(task_t* new_task) {
	scheduler_t* scheduler = sched_get();
	if (inside_main()) {
		debug("Adding a task from main");
		queue_push(&scheduler->ready,new_task);
	} else {
		scheduler->new_task = new_task;
		scheduler->action = ADD_TASK;	
		sched_invoke();
	}	
}

void sched_yield_current() {
	sched_get()->action = YIELD;
	sched_invoke();
}

void sched_invoke() {
	scheduler_t* scheduler = sched_get();
	if (!scheduler) {
		log_err("NO SCHEDULER");
	}
	if (scheduler->current_task != NULL) {
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
	if (*id == -1 ) {
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

/**
 * Initialize the necessary tasks
 */
void task_init(int workers) {
	schedulers_nr = workers;
	main_id = (unsigned int) pthread_self();
	start_program();
	sched_init(workers);
}

/**
 * Perform cleanup.
 */
void task_deinit(void * arg) {
	debug("%d - Thread exiting...", (unsigned int) pthread_self());		
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
	pthread_mutex_init(&new_task->running_lock, NULL);
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
	// if (!inside_main()) {
		scheduler_t* scheduler = sched_get();
		scheduler->action = RETURN_TASK;
	setcontext(scheduler->context);
		//sched_invoke();
	// } else {
	// 	debug("Returning from main");
	// }
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
	pthread_mutex_lock(&program.lock);
	int state = 0;
	if (PROGRAM_ENDED == program.state ) {
		state = 1;
	}
	pthread_mutex_unlock(&program.lock);
	return state;
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
	return ((unsigned int) pthread_self() == main_id) ? 1 : 0;
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
			}
		}		
		if (someone_not_done) {
			if (!inside_main()) {
				debug("%d - Yielding...", sched_get()->id);
				sched_yield_current();
			} else {
				debug("Main waiting");
				pthread_mutex_lock(&wake_up_main_lock);
				pthread_cond_wait(&wake_up_main, &wake_up_main_lock);
				pthread_mutex_unlock(&wake_up_main_lock);
				debug("Main woken up");
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
		debug("Going outside of sync in main %d", (unsigned int ) pthread_self());
	} else {
		debug("Going outside of sync in task %d", task_current()->id);
	}
	
	return 0;
}

void task_destroy(task_t* task) {
	if (task) {
		pthread_mutex_destroy(&task->lock);
		pthread_mutex_destroy(&task->running_lock);
		SAFE_FREE(task->context->uc_stack.ss_sp);
		SAFE_FREE(task->context);
		SAFE_FREE(task);	
	}
}

int task_first_child(task_t* task) {
	return -1;
}

void task_set_status(task_t* task, status_t new_status) {	
	pthread_mutex_lock(&task->lock);
	task->status = new_status;
	pthread_mutex_unlock(&task->lock);
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

