/*
 * utils.c
 *
 *  Created on: Oct 22, 2013
 *      Author: bogdan
 */
#include <math.h>
#include <stdlib.h>
#include "utils.h"

stats_t compute_stats(double* values, int size) {
	stats_t stats;
	double mean = 0, variance = 0;

	int i;
	for (i = 0; i < size; i++) {
		mean+= values[i];
	}
	stats.mean = mean / size;
	for (i = 0; i < size; i++) {
		variance += pow((stats.mean - values[i]), 2);
	}
	stats.standard_deviation = sqrt(variance/size);
	return stats;
}
