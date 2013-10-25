/*
 * ucontext_test.c
 *
 *  Created on: Oct 22, 2013
 *      Author: bogdan
 */
#include <stdio.h>
#include <ucontext.h>
#include <stdlib.h>
#include <sys/time.h>
#include "utils.h"
#define EXPERIMENTS 10

#define STACK_SIZE	4 * 1028
#define ALLOCATE_STACK_UC(context)	ALLOCATE_STACK(context.uc_stack)
#define ALLOCATE_STACK(stack) 	stack.ss_sp = malloc(STACK_SIZE); \
								stack.ss_size = STACK_SIZE
#define FREE_STACK_UC(context)  FREE_STACK(context.uc_stack)
#define FREE_STACK(stack)		free(stack.ss_sp)
#define LINK(child, parent)		child.uc_link = &parent;

#define TASKS 2
ucontext_t main_t, first_t, second_t;

volatile int counter = 0;
int switches;
int experiments;

int main(int argc, char **argv) {
	counter = 0;
	printf("Usage: ./ucontext_test nr_switches nr_experiments\n");
	if (argc < 3) {
		switches = SWITCHES;
		experiments = EXPERIMENTS;
	} else {
		switches = atoi(argv[1]);
		experiments = atoi(argv[2]);
	}

	double* experiment_results = (double*) calloc(experiments,sizeof(double));

	int i;
	for (i = 0; i < experiments; i++) {
		double running_time =  switch_time_ucontext();
		printf("Total time for %d switches: %f s\n", switches, running_time);
		printf("Time per switch %f ns\n",  TO_NS(running_time/switches));
		experiment_results[i] =  TO_NS(running_time/switches);
	}

	stats_t stats = compute_stats(experiment_results, experiments);
	printf("Mean %f ns\n", stats.mean);
	printf("Standard deviation: %f ns\n", stats.standard_deviation);

	free(experiment_results);
	return 0;
}

double switch_time_ucontext() {
	counter = 0;
	initialize_tasks();

	struct timeval start,end;
	gettimeofday(&start, 0);
	if (counter < switches) {
		first();
	}
	gettimeofday(&end, 0);
	deinitialize_tasks();
	double running_time = timeval_diff(start, end);

	//now let's measure the running of only the memory accesses
	gettimeofday(&start, 0);
	counter = 0;
	while (++counter < switches);
	gettimeofday(&end, 0);
	double memory_access = 0;

	return running_time - memory_access;
}

int first() {
	getcontext(&first_t);
	if (counter == 0) {
		second();
	}
	getcontext(&first_t);

	if (++counter < switches) {
		//swapcontext(&first_t, &second_t);
		setcontext(&second_t);			//setcontext is faster since it doesn't save the current context
	}
	return 0;
}

int second() {
	getcontext(&second_t);

	if (++counter < switches) {
		//swapcontext(&second_t, &first_t);
		setcontext(&first_t);
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
	printf("Initialized tasks\n");
}

void deinitialize_tasks() {
	FREE_STACK_UC(first_t);
	FREE_STACK_UC(second_t);
	FREE_STACK_UC(main_t);
}


