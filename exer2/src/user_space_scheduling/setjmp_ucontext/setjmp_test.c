#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <sys/time.h>
#include "utils.h"

#define EXPERIMENTS 10
/**
 * Variable declarations
 */
jmp_buf main_task,first_task,second_task;

volatile int counter = 0;

int switches;
int experiments;

int main(int argc, char **argv) {
	counter = 0;

	printf("Usage: ./setjmp_test nr_switches nr_experiments\n");
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
		double running_time =  switch_time_setjmp();
		printf("Total time for %d switches: %f s\n", switches, running_time);
		printf("Time per switch %f ns\n", TO_NS(running_time/switches));
		experiment_results[i] =  TO_NS(running_time/switches);
	}

	stats_t stats = compute_stats(experiment_results, experiments);
	printf("Mean %f ns\n", stats.mean);
	printf("Standard deviation: %f ns\n", stats.standard_deviation);

	free(experiment_results);
	return 0;
}

/**
 * Function definitions
 */

double switch_time_setjmp() {
	//making sure the function has enough stack space
	ALLOCATE_CUSHION(CUSHION_SIZE);

	counter = 0;
	struct timeval start,end;
	gettimeofday(&start, 0);
	if (!setjmp(main_task)) {
		first();
	}
	gettimeofday(&end, 0);

	//added so the compiler doesn't optimize the cushion out of existence
	DUMMY_USE_CUSHION(CUSHION_SIZE);
	double running_time = timeval_diff(start, end);

	//now let's measure the running of only the memory accesses
	gettimeofday(&start, 0);
	counter = 0;
	while (++counter < switches);
	gettimeofday(&end, 0);
	double memory_access = timeval_diff(start, end);

	return running_time - memory_access;
}

int first() {
	/* Allocated a cushion for this function */
	ALLOCATE_CUSHION(CUSHION_SIZE);

	if (!setjmp(first_task)) {
		second();
	}

	setjmp(first_task);

	if ( ++counter < switches) {
		longjmp(second_task,1);
	}

	longjmp(main_task,1);

	DUMMY_USE_CUSHION(CUSHION_SIZE);
	return 0;
}


int second() {
	ALLOCATE_CUSHION(CUSHION_SIZE);

	setjmp(second_task);

	if (++counter < switches) {
		longjmp(first_task,1);
	}

	longjmp(main_task,1);

	DUMMY_USE_CUSHION(CUSHION_SIZE);

	return 0;
}





