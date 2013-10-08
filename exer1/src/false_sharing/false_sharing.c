#define _GNU_SOURCE
#include <sys/times.h>
#include <time.h>
#include <stdio.h> 
#include <pthread.h> 
#include <sched.h>
#include "false_sharing.h"

#define ITERATIONS 1000000000
#define ADDED_VALUE 1
long long int tmsBegin1, tmsEnd1, tmsBegin2, tmsEnd2, tmsBegin3, tmsEnd3;

int array[100];

int main(int argc, char *argv[]) {
	int first_elem = 0;
	int bad_elem = 1;
	int good_elem = 32;
	double same_line_time = measure_memory_access(first_elem, bad_elem);
	printf("Time needed for accessing values in the same cache line: %f\n",same_line_time);
	check_array_value(first_elem, ADDED_VALUE * ITERATIONS);
	check_array_value(bad_elem, ADDED_VALUE * ITERATIONS);
	double different_line_time = measure_memory_access(first_elem, good_elem);
	printf("Time needed for accessing values in different cache line: %f\n",different_line_time);
	check_array_value(first_elem, ADDED_VALUE * ITERATIONS);
	check_array_value(good_elem, ADDED_VALUE * ITERATIONS);
	return 0;
}

void bind_to_cpu(int cpu) {
	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET(cpu, &mask);
	pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &mask);
}

void heavy_loop(void *args) {
	arguments* a = (arguments *) args;
	bind_to_cpu(a->cpu);
	int i;
	for (i = 0; i < ITERATIONS; i++) {
		array[a->index] += ADDED_VALUE;
	}
}

double measure_memory_access(int index1, int index2) {
	pthread_t thread_1, thread_2;
	arguments args[2];
	array[index1] = array[index2] = 0;			//Reset values

	args[0].index = index1;
	args[0].cpu = 0;
	args[1].index = index2;
	args[1].cpu = 1;
	clock_t start = clock();
	pthread_create(&thread_1, NULL, (void*) heavy_loop, (void*) &args[0]);
	pthread_create(&thread_2, NULL, (void*) heavy_loop, (void*) &args[1]);
	pthread_join(thread_1, NULL);
	pthread_join(thread_2, NULL);

	clock_t end = clock();
	return (double) (end - start) / CLOCKS_PER_SEC;
}

void check_array_value(int index, int expected_value) {
	if (array[index] == expected_value) {
		printf("Element %d of the array has the CORRECT value %d.\n", index, expected_value);
	} else {
		printf("Element %d of the array has value %d when it should have %d.\n", index, array[index], expected_value);
	}
}
