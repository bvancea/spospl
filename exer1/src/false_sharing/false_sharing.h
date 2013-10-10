/*
 * false_sharing.h
 *
 *  Created on: Oct 8, 2013
 *      Author: bogdan
 */

#ifndef FALSE_SHARING_H_
#define FALSE_SHARING_H_

#include <sys/times.h>
#include <time.h>
#include <stdio.h>
#include <pthread.h>
#include <sched.h>
#include <stdlib.h>
#include <linux/types.h>

typedef struct argument_t {
	int index;
	int cpu;
} arguments;

void check_array_values(int thread_nr, int same_cache_line, int expected_value);
void check_array_value(int index, int expected_value);
void print_array(int* array, int size);
void read_parameters();

double measure_memory_access();
void heavy_loop(void *args);
void bind_to_cpu(int cpu);
#endif /* FALSE_SHARING_H_ */
