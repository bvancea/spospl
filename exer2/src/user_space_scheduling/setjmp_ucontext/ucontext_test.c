/*
 * ucontext_test.c
 *
 *  Created on: Oct 22, 2013
 *      Author: bogdan
 */
#include <stdio.h>
#include <ucontext.h>
#include <stdlib.h>
#include <time.h>
#include "utils.h"
#define EXPERIMENTS 10

#define STACK_SIZE	4 * 1028
#define ALLOCATE_STACK_UC(context)	ALLOCATE_STACK(context.uc_stack)
#define ALLOCATE_STACK(stack) 	stack.ss_sp = malloc(STACK_SIZE); \
								stack.ss_size = STACK_SIZE; \
								stack.ss_flags = 0
#define FREE_STACK_UC(context)  FREE_STACK(context.uc_stack)
#define FREE_STACK(stack)		free(stack.ss_sp)
#define LINK(child, parent)		child.uc_link = &parent;

#define TASKS 2
ucontext_t main_t, first_t, second_t;

volatile int counter = 0;

int main(int argc, char **argv) {
	double* experiment_results = (double*) calloc(EXPERIMENTS,sizeof(double));
	int i;
	for (i = 0; i < EXPERIMENTS; i++) {
		double running_time =  switch_time_ucontext(SWITCHES);
		printf("Total time for %d switches: %f s\n", SWITCHES, running_time);
		printf("Time per switch %f ns\n", TO_NS(running_time)/SWITCHES);
		experiment_results[i] = TO_NS(running_time)/SWITCHES;
	}

	stats_t stats = compute_stats(experiment_results, EXPERIMENTS);
	printf("Mean %f ns\n", stats.mean);
	printf("Standard deviation: %f ns\n", stats.standard_deviation);

	free(experiment_results);
	return 0;
}

double switch_time_ucontext(int iterations) {
	counter = 0;
	initialize_tasks();
	printf("Starting in main\n");
	clock_t start = clock();
	getcontext(&main_t);
	if (counter < SWITCHES) {
		printf("Jumping to first\n");
		first();
	}
	clock_t end = clock();
	deinitialize_tasks();
	double running_time =  ((double) end - start) / CLOCKS_PER_SEC;
	return running_time;
}

int first() {
	getcontext(&first_t);
	if (counter == 0) {
		second();
	}
	getcontext(&first_t);

	while (++counter < SWITCHES) {
		swapcontext(&first_t, &second_t);
	}
	return 0;
}

int second() {
	getcontext(&second_t);

	while (++counter > SWITCHES) {
		swapcontext(&second_t, &first_t);
	}
	return 0;
}

void initialize_tasks() {
	ALLOCATE_STACK(main_t.uc_stack);

	//allocate first task
	ALLOCATE_STACK_UC(first_t);
	LINK(first_t, main_t);
	makecontext(&first_t, (void (*)(void)) first, 1, 0);

	//allocate second task
	ALLOCATE_STACK_UC(second_t);
	LINK(second_t, main_t);
	makecontext(&second_t, (void (*)(void)) second, 1, 0);
}

void deinitialize_tasks() {
	FREE_STACK_UC(first_t);
	FREE_STACK_UC(second_t);
	FREE_STACK_UC(main_t);
}


