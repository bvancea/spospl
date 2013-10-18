/*
 * rts_api.h
 *
 *  Created on: Oct 16, 2013
 *      Author: bogdan
 */

#ifndef RTS_API_H_
#define RTS_API_H_

/**
 * Identifies a single execution context.
 */
typedef struct context {
	void* context;
}context_t;

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
int spawn_worker(context_t* execution_context, void* fct_ptr, void* arguments);


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
int sync_worker(context_t* execution_context, void *return_val);

#endif /* RTS_API_H_ */
