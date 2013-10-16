/*
 * fibo.c
 *
 *  Created on: Oct 16, 2013
 *      Author: bogdan
 */
#include "rts_api.h"

int fib(int n) {

	if (n < 2) {
		return n;
	}
	int *x, *y;
	//create 2 contexts for the child functions
	context_t ct_x, ct_y;

	//execute the child functions in different execution contexts
	x = (int *) span_worker(&ct_x, (void) &fib, (void*) (n-1), 1);
	y = (int *) span_worker(&ct_y, (void) &fib, (void*) (n-2), 1);

	//wait for the child contexts to complete
	sync_worker(&ct_x);
	sync_worker(&ct_y);

	//return the result
	return (*x + *y);
}

