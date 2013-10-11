#define _GNU_SOURCE

#include "false_sharing.h"

#define ITERATIONS 1000000000
#define ADDED_VALUE 1
long long int tmsBegin1, tmsEnd1, tmsBegin2, tmsEnd2, tmsBegin3, tmsEnd3;

int array[100];
int thread_nr,same_cache_line;
int* cpu_for_thread;

int main(int argc, char *argv[]) {
	read_parameters();
	print_array(cpu_for_thread, thread_nr);
	double access_time = measure_memory_access();
	printf("Time needed for accessing array values: %f\n",access_time);

	check_array_values(thread_nr, same_cache_line, (int)ADDED_VALUE * ITERATIONS);

	free(cpu_for_thread);
	return 0;
}

void read_parameters() {
	do {
		printf("Please enter number of threads:");
	} while (!scanf("%d", &thread_nr));

	cpu_for_thread = (int*) malloc(thread_nr * sizeof(int));
	int i;
	for (i = 0; i < thread_nr; ++i) {
		do {
			printf("\nEnter cpu for thread %d: ", i);
		} while (!scanf("%d", cpu_for_thread + i));
	}
	do {
		printf("\nSame cache line? 1 for true, 0 for false. ");
	} while (!scanf("%d", &same_cache_line));
}

void bind_to_cpu(int cpu) {
	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET(cpu, &mask);
	pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &mask);
}

/**
 *	Binds the current thread to a specific CPU and then performs a loop increment
 *	on an array value.
 */
void heavy_loop(void *args) {
	arguments* a = (arguments *) args;
	bind_to_cpu(a->cpu);
	int i;
	for (i = 0; i < ITERATIONS; ++i) {
		array[a->index] += ADDED_VALUE;
	}
}

/**
 * Create a number of threads to increment memory locations.
 */
double measure_memory_access() {
	printf("Starting measurements.\n");
	pthread_t* threads = (pthread_t*)malloc(thread_nr * sizeof(pthread_t));
	arguments* args = (arguments*) malloc(thread_nr * sizeof(int));

	clock_t start = clock();
	int i;
	for (i = 0; i < thread_nr; ++i) {
		if (same_cache_line) {
			args[i].index = i;
		} else {
			args[i].index = i * 8;
		}
		args[i].cpu = cpu_for_thread[i];
		printf("Thread %d running on cpu %d and accessing index %i started.\n", i, args[i].cpu, args[i].index);
		pthread_create(&threads[i], NULL, (void*) heavy_loop,(void*) &args[i]);
	}

	for (i = 0; i < thread_nr; ++i) {
		pthread_join(threads[i], NULL);
		printf("Thread %d stopped.\n", i);
	}

	clock_t end = clock();

	return (double) (end - start) / CLOCKS_PER_SEC;
}

/**
 * Check if the addition results are valid.
 */
void check_array_values(int thread_nr, int same_cache_line, int expected_value) {
	int i;
	for (i = 0; i < thread_nr; ++i) {
		if (same_cache_line) {
			check_array_value(i,expected_value);
		} else {
			check_array_value(i * 8,expected_value);
		}
	}
}

void check_array_value(int index, int expected_value) {
	if (array[index] == expected_value) {
		printf("Element %d of the array has the CORRECT value %d.\n", index, expected_value);
	} else {
		printf("Element %d of the array has value %d when it should have %d.\n", index, array[index], expected_value);
	}
}


void print_array(int* arr, int size) {
	int i;
	for (i = 0; i < size; i++) {
		printf("%d ",*(arr + i) );
	}
	printf("\n");
}
