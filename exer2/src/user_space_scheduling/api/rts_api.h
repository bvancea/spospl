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
typedef struct context context_t;

/**
 * Run the function pointed by fct_ptr with the given arguments in a different execution context.
 *
 * Arguments:
 *
 * execution_context	: 	a pointer to an execution context used for this task
 * fct_ptr				:	a pointer to the function executed for this task
 * arguments			: 	an array of arguments which should be passed to the function
 * argc					:	the number of arguments in the array
 *
 * returns				:	a void pointer containing the address of the result
 */
void* span_worker(context_t* execution_context, void* fct_ptr, void* arguments, int argc);

/**
 * Barrier method, used to make the current thread wait for the completion of the execution context
 * received as argument.
 *
 * Arguments
 *
 * execution_context	: 	a pointer to an execution context used for this task *
 */
void* sync_worker(context_t* execution_context);

#endif /* RTS_API_H_ */
