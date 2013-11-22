/*
 * rts_api.h
 *
 *  Created on: Oct 16, 2013
 *      Author: bogdan
 */

#ifndef TASK_H_
#define TASK_H_
#include <ucontext.h>
#include <pthread.h>

#define QUEUE_SIZE 8 * 1024

typedef void (*function_t)(void *);

typedef enum sched_action { IDLE, ADD_TASK, YIELD, RETURN_TASK } sched_action_t;
typedef enum status { STARTED, COMPLETED } status_t;

typedef enum program_state {PROGRAM_STARTED, PROGRAM_ENDED} program_state_t;

// this will probably be changed a little
typedef struct context {
	int id;
	ucontext_t* context;
	status_t status;

	function_t function;
	void* result;
	void* arguments;
	
	int children;
	struct context* parent;

	pthread_mutex_t lock;
} task_t;

typedef struct queue {
	task_t* tasks[QUEUE_SIZE];
	int top;
	
	pthread_mutex_t lock;
} queue_t;

typedef struct scheduler {
	ucontext_t* context;
	pthread_mutex_t lock;
	
	int id;

	task_t* new_task;
	sched_action_t action;

	//TODO cleanup
	task_t* current_task;
	/* Tasks ready to execute */
	//list_t ready;	
	queue_t ready;

	pthread_t thread;
} scheduler_t;

typedef struct parallel_program {
	program_state_t state;
	pthread_mutex_t lock;
} program_t;

/**
 * Initializes the task API.
 *
 * Allocates a stack for the scheduler.
 */
void task_init(int nr_threads);

/**
 * Perform clean-up.
 */
void task_deinit(void*);
/**
 * Run the function pointed by fct_ptr with the given arguments in a different execution context.
 *
 * Arguments:
 *
 * execution_context	: 	a pointer to an execution context used for this task
 * fct_ptr				:	a pointer to the function executed for this task
 * arguments			: 	a void pointer to the arguments of the function.
 * 							This could be a pointer to a structure or a single variable, similar to the pthread library.
 *
 * returns				:	0 if the call is successful, otherwise an error number
 */
task_t* task_spawn(void* fct_ptr, void* arguments, void *return_val);


/**
 * Barrier method, used to make the current thread wait for the completion of the execution context
 * received as argument.
 *
 * Arguments
 *
 * execution_context	:  a pointer to an execution context used for this task
 * return_val			:  a pointer to an address where the result returned by the task is.
 *
 * returns				:  0 if the call is successful, otherwise an error number
 */
int task_sync(task_t** execution_context, int count);

/**
 * Release control to the scheduler which will mark the current task a finished.
 *
 * The task cannot be run anymore.
 */
int task_return();

void task_destroy(task_t* task);

#define RETURN(value, type) task_t* current_task = task_current();			\
							*((type*)current_task->result) = *value; 		\
							task_return()

/**
 * Returns a pointer to the currently running task.
 */
task_t* task_current();

void task_inc_children_count(task_t* task);
void task_dec_children_count(task_t* task);

void* thread_init(void* arg);
void task_end();

void worker_init(int workers);
void* worker_loop();
void sched_init();
void* sched_execute_action(void *arguments);
void sched_add_task(task_t* new_task);
void sched_yield_current();
void sched_invoke();
void sched_wrapper_function(void* c);
scheduler_t* sched_get();

void sched_handler_add_task(scheduler_t* scheduler);
void sched_handler_yield(scheduler_t* scheduler);
void sched_handler_return(scheduler_t* scheduler);
void sched_handler_try_steal(scheduler_t* scheduler);
void sched_handler_execute(scheduler_t* scheduler);

int has_program_ended();
int inside_main();

void start_program();
void end_program(); 
#endif /* RTS_API_H_ */
