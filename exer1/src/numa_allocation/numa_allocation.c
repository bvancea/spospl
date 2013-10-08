/*
 * numa_allocation.c
 */
#define _GNU_SOURCE
#include <numa.h>
#include <stdio.h>
#include <sched.h>
#include "numa_allocation.h"



int main(int argc, char **argv) {

	if (numa_available()) {
		printf("Numa available.\n");
	} else {
		printf("Numa not available.\n");
	}

	check_topology();

	numa_set_localalloc();

	return 0;
}

void bindToNode(int node) {

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
 * Uses the same method described in the
 */
void bind_to_numa_node(int node) {
	nodemask_t node_mask;
	nodemask_zero(&node_mask);
	/*nodemask_set(&node_mask, node);
	numa_bind(&node_mask);*/
}

void bind_to_cpu(int cpu) {

}


