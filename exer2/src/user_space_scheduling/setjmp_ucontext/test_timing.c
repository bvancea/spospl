/*
 * test_timing.c
 *
 *  Created on: Oct 25, 2013
 *      Author: bogdan
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <setjmp.h>
#include <ucontext.h>

volatile int counter = 0;

double timeval_diff(struct timeval begin, struct timeval end) {
	double elapsed = (end.tv_sec - begin.tv_sec) + ((end.tv_usec - begin.tv_usec)/1000000.0);
	return elapsed;
}

int main(int argc, char **argv) {

	struct timeval start, end;
	int volatile i;
	gettimeofday(&start, NULL);

	for (i = 0; i < 1000; i++);

	gettimeofday(&end, NULL);

	double memory_time = 1000 * 1000 * timeval_diff(start, end);
	printf("Time for memory access: %f ns\n", memory_time);

	jmp_buf buf;
	gettimeofday(&start, NULL);

	i = 0;
	setjmp(buf);
	if (i < 1000) {
		i++;
		longjmp(buf,1);
	}

	gettimeofday(&end, NULL);
	printf("Time for longjmp/setjmp + memory access: %f ns\n", 1000 * 1000 * timeval_diff(start, end) - memory_time);


	ucontext_t context;
	gettimeofday(&start, NULL);

	i = 0;
	getcontext(&context);
	if (i < 1000) {
		i++;
		setcontext(&context);
	}

	gettimeofday(&end, NULL);
	printf("Time for getcontext/swapcontext + memory access: %f ns\n", 1000 * 1000 * timeval_diff(start, end) - memory_time);
	return 0;
}
