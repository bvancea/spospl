/*
 * numa_allocation.c
 */
#define _GNU_SOURCE
#include <numa.h>
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <assert.h>
#include "numa_allocation.h"

#define ARRAY_SIZE 100000000

void read_parameters(int argc, char **argv, int* node, int* cpu) {
	assert(argc == 3 && "First parameter is the memory node and the second parameter is the CPU used to run ");
	*node = atoi(argv[1]);
	*cpu = atoi(argv[2]);
	fprintf(stdout, "Allocating memory on node: %d\nExecuting program on CPU: %d.", *node, *cpu);
	if (numa_node_of_cpu(*cpu) == *node) {
		fprintf(stdout, "Based on the topology, testing local memory access\n");
	} else {
		fprintf(stdout, "Based on the topology, testing remote memory access.\n");
	}
}

int main(int argc, char **argv) {
	int node, cpu;
	check_topology();

	read_parameters(argc, argv, &node, &cpu);

	bind_to_cpu(cpu);

	int* array = (int*) numa_alloc_onnode(ARRAY_SIZE * sizeof(int), node);
	generate_random_array(array, ARRAY_SIZE);
	clear_cache();
	memory_read_latency(array, ARRAY_SIZE, ARRAY_SIZE);

	return 0;
}

void check_topology(void) {
	int cpus = numa_num_task_cpus();
	int nodes = numa_num_task_nodes();

	printf("######### Numa Topology ###########\n");
	printf("Number of CPU's: %d\n", cpus);
	printf("Number of nodes: %d\n", nodes);

	struct bitmask *bm = numa_bitmask_alloc(cpus);
	int i;
	for (i = 0; i <= numa_max_node(); ++i) {
		numa_node_to_cpus(i,bm);
		printf("Node %d CPU's: ",i);
		cpus_from_bitmask(bm);
		printf("Memory: %d GB\n", node_size_in_GB(i));
	}
	printf("##################################\n");

}

int node_size_in_GB(int node) {
	return numa_node_size(node,0)/ (1024 * 1024 * 1024);
}

int* cpus_from_bitmask(struct bitmask *bm) {
	int* cpu_ids = (int* ) malloc(bm->size * sizeof(int));
	int i, index = 0;
	for(i = 0; i< bm->size; ++i) {
		if (numa_bitmask_isbitset(bm, i)) {
			cpu_ids[index++] = i;
		}
	}
	for(i = 0; i < index; i++) {
		printf("%d ", cpu_ids[i]);
	}
	return cpu_ids;
}

int current_numa_node() {
	int cpu = sched_getcpu();
	return numa_node_of_cpu(cpu);
}

/**
 * 	Allocate a large array and access it to make sure the contents of the cache are replaced.
 */
void clear_cache() {
	long long int i = 0;
	int* array = (int*) malloc(ARRAY_SIZE * sizeof(int));
	for (i = 0; i < ARRAY_SIZE; i++) {
		array[i] += 1;
	}
	free(array);
}

/**
 * Binds the current thread to the CPU cpu.
 */
void bind_to_cpu(int cpu) {
	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET(cpu, &mask);
	pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &mask);
}

void* generate_random_array(int* array, int size) {
	fprintf(stdout, "Generating array of integers in the range 0 - %d...\n", size);
	int i = 0;
	for (i = 0; i < size; i++) {
		array[i] = i;
	}
	shuffle(array, size);
	return array;
}

void shuffle(int *array, int n) {
	fprintf(stdout, "Shuffling array...\n");
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int usec = tv.tv_usec;
    srand48(usec);
    if (n > 1) {
        int i;
        for (i = n - 1; i > 0; i--) {
            int j = (unsigned int) (drand48()*(i+1));
            swap(&array[i], &array[j]);
        }
    }
}

void swap(int* a, int *b) {
	int temp = *a;
	*a = *b;
	*b = temp;
}

void print_array(int* array, int size) {
	int i;
	for (i = 0; i < size; i++) {
		printf("%d ",array[i] );
	}
	printf("\n");
}

/**
 * Will fit into an into on modern cpus, since tv_user guaranteed to be less than 1 million.
 */
int get_current_time_ns() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_usec * 1e3;
}

void memory_read_latency(int* array, int size, int steps) {
	struct timeval t0,t1;
	gettimeofday(&t0, 0);
	int i;
	int j = 0;
	long long int sum;
	for (i = 0; i < steps; i++) {
		j = array[j]++;
		sum += j;
	}
	gettimeofday(&t1, 0);
	long long int elapsed = (t1.tv_sec-t0.tv_sec)*1000000 + t1.tv_usec-t0.tv_usec;
	printf("\nMemory latency: %f ns\n", (elapsed * 1e3) / steps);
}
