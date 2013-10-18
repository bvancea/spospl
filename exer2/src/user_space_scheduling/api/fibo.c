/*
 * fibo.c
 *
 *  Created on: Oct 16, 2013
 *      Author: bogdan
 */
#include "rts_api.h"
#include <stdlib.h>

#define ALLOCATE_INT(x) x = (int *) malloc(sizeof(int));

int fib(int n) {

        if (n < 2) {
                return n;
        }
        int *x, *y, *arg_x, *arg_y;
        ALLOCATE_INT(x);
        ALLOCATE_INT(y);
        ALLOCATE_INT(arg_x);
        ALLOCATE_INT(arg_y);

        //create 2 contexts for the child functions
        context_t ct_x, ct_y;

        *x = n - 1;
        *y = n - 2;
        //execute the child functions in different execution contexts
        spawn_worker(&ct_x, (void*) &fib, (void*) arg_x);
        spawn_worker(&ct_y, (void*) &fib, (void*) arg_y);

        //wait for the child contexts to complete
        sync_worker(&ct_x, (void*) x);
        sync_worker(&ct_y, (void*) y);

        //return the result
        return (*x + *y);
}
