/*
 * utils.h
 *
 *  Created on: Oct 22, 2013
 *      Author: bogdan
 */

#ifndef UTILS_H_
#define UTILS_H_

#define SWITCHES 10000000
#define CUSHION_SIZE 2000

/**
 * Preprocessor macros
 */
#define ALLOCATE_CUSHION(size) int cushion[size]; \
								cushion[size-1] = 1;

#define DUMMY_USE_CUSHION(size) int i;	\
								for (i = 1; i < 2000; i++) { \
									cushion[i] = i + cushion[i-1]; \
								}

#define TO_MS(seconds) 1000*seconds
#define TO_US(seconds) 1000*TO_MS(seconds)
#define TO_NS(seconds) 1000*TO_US(seconds)

typedef struct stats {
	double mean;
	double standard_deviation;
} stats_t;

stats_t compute_stats(double* values, int size);

/**
 * Function declarations
 */
double switch_time_setjmp(int iterations);
int first();
int second();

/**
 * U-context
 */
double switch_time_ucontext(int iterations);
void initialize_tasks();
void deinitialize_tasks();

#endif /* UTILS_H_ */
