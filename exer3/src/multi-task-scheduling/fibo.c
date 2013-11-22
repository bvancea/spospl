/*
 * fibo.c
 *
 *  Created on: Oct 16, 2013
 *      Author: bogdan
 */
#include <stdlib.h>
#include <unistd.h>
#include "./task.h"
#include "./dbg.h"
#include <sys/resource.h>

#define ALLOCATE_INT(x) x = (int *) malloc(sizeof(int));
#define ALLOCATE_INT_VALUE(x,v) ALLOCATE_INT(x); \
								*x = v

void fibo(void* args) {
	int n = *((int *) args);
	int returned = n;
	if (n >= 2) {
		int x,y,arg_x = n-1, arg_y = n-2;
		//create 2 contexts for the child functions
		task_t* ct[2];

		//execute the child functions in different execution contexts
		ct[0] = task_spawn((void*) &fibo, &arg_x, &x);
		ct[1] = task_spawn((void*) &fibo, &arg_y, &y);

		//wait for the child contexts to complete
		task_sync(ct, 2);
		returned = x + y;
	}

	RETURN(&returned, int);
}

int fib(int n) {
	int result;

	task_t* t[1];
	t[0] = task_spawn((void*) fibo, &n, &result);
	task_sync(t,1);
	return result;
}

int main(int argc, char **argv) {
	int n = 10;
	if (argc > 1) {
		n = atoi(argv[1]);
	}

	int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
	printf("Detected %d CPU cores.", num_cores);
	task_init(num_cores);	
	int r = fib(5);
	printf("Fibo %d is %d\n", n, r);
	task_end();
	return 0;
}

