#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <time.h>
#include "utils.h"

#define EXPERIMENTS 10
/**
 * Variable declarations
 */
jmp_buf main_task,first_task,second_task;
int volatile counter;


int main(int argc, char **argv) {
	counter = 0;

	double* experiment_results = (double*) calloc(EXPERIMENTS,sizeof(double));
	int i;
	for (i = 0; i < EXPERIMENTS; i++) {
		double running_time =  switch_time_setjmp(SWITCHES);
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

/**
 * Function definitions
 */

double switch_time_setjmp(int iterations) {
	ALLOCATE_CUSHION(CUSHION_SIZE);

	counter = 0;
	clock_t start = clock();
	if (!setjmp(main_task)) {
		first(iterations);
	}
	clock_t end = clock();

	DUMMY_USE_CUSHION(CUSHION_SIZE);

	double running_time =  ((double) end - start) / CLOCKS_PER_SEC;
	return running_time;
}

int first() {
	/* Allocated a cushion for this function */
	ALLOCATE_CUSHION(CUSHION_SIZE);

	if (!setjmp(first_task)) {
		second();
	}

	setjmp(first_task);

	if ( ++counter > SWITCHES) {
		longjmp(main_task,1);
	} else {
		longjmp(second_task,1);
	}

	DUMMY_USE_CUSHION(CUSHION_SIZE);
	return 0;
}


int second() {
	ALLOCATE_CUSHION(CUSHION_SIZE);

	setjmp(second_task);
	if (++counter > SWITCHES) {
		longjmp(main_task,1);
	} else {
		longjmp(first_task,1);
	}

	DUMMY_USE_CUSHION(CUSHION_SIZE);

	return 0;
}




