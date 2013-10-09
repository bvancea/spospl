/*
 * numa_allocation.h
 *
 *  Created on: Oct 7, 2013
 *      Author: bogdan
 */

#ifndef NUMA_ALLOCATION_H_
#define NUMA_ALLOCATION_H_

typedef struct numa_node_t {
	int node;
	int* cpu_ids;
} numa_node;

void check_topology(void);
void memory_read_latency(int* array, int size, int steps);
void clear_cache();
void* generate_random_array(int* array, int size);
void swap(int* a, int *b);
void shuffle(int *array, int n);
void print_array(int* array, int size);

int current_numa_node();
int node_size_in_GB(int node);
int* cpus_from_bitmask(struct bitmask *bm) ;
void bind_to_cpu(int cpu);

#endif /* NUMA_ALLOCATION_H_ */
