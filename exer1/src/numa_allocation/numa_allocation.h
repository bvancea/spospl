/*
 * numa_allocation.h
 *
 *  Created on: Oct 7, 2013
 *      Author: bogdan
 */

#ifndef NUMA_ALLOCATION_H_
#define NUMA_ALLOCATION_H_

void check_topology(void);

int current_numa_node();
int node_size_in_GB(int node);
int* cpus_from_bitmask(struct bitmask *bm) ;

void bind_to_cpu(int cpu);
void bind_to_numa_node(int node);

#endif /* NUMA_ALLOCATION_H_ */
