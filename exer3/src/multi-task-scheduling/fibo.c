/*
 * fibo.c
 *
 *  Created on: Oct 16, 2013
 *      Author: bogdan
 */
#include <stdlib.h>
#include "./task.h"
#include "./lists.h"
#include "./dbg.h"


#define ALLOCATE_INT(x) x = (int *) malloc(sizeof(int));
#define ALLOCATE_INT_VALUE(x,v) ALLOCATE_INT(x); \
								*x = v

void fibo(void* args) {
	int n = *((int *) args);
	debug("Executing fibo %d", n);
	int* returned;
	ALLOCATE_INT_VALUE(returned, n);

	if (n >= 2) {
		int *x, *y, *arg_x, *arg_y;
		ALLOCATE_INT(x);
		ALLOCATE_INT(y);
		ALLOCATE_INT_VALUE(arg_x, n-1);
		ALLOCATE_INT_VALUE(arg_y, n-2);

		//create 2 contexts for the child functions
		task_t* ct[2];

		//execute the child functions in different execution contexts
		ct[0] = task_spawn((void*) &fibo, arg_x, x);
		ct[1] = task_spawn((void*) &fibo, arg_y, y);

		//wait for the child contexts to complete
		task_sync(ct, 2);

		//return the result
		*returned = *x + *y;

		free(x);
		free(y);
		free(arg_x);
		free(arg_y);
	}

	RETURN(returned, int);
}

int fib(int n) {
	int result;

	task_t* t[1];
	t[0] = task_spawn((void*) fibo, &n, &result);
	task_sync(t,1);
	return result;
}


int main(int argc, char **argv) {
	task_init(2);
	int n = 23;
	int r = fib(n);
	printf("Fibo %d is %d\n", n, r);
	return 0;
}

