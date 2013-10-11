#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include "random_generator.h"

int main(int argc, char **argv) {

	int t,n;
	if (argc < 3) {
		printf("The first parameter is the number of threads, the second is the number of iterations.\n");
		t = (int) sysconf( _SC_NPROCESSORS_ONLN );
		n = (int) DEFAULT_N;
	} else {
		t = atoi(argv[1]);
		n = atoi(argv[2]);
	}
	printf("Running with parameters: %d (number of CPU's on the system) and %d iterations.\n", t, n);
	pthread_t* threads = malloc(t * sizeof(pthread_t));
	createWorkerThreads(threads, t,n);

	return 0;
}

void createWorkerThreads(pthread_t* threads, int t, int n) {
	clock_t begin = clock();
	int i;
	printf("All threads starting..\n");
	arguments* arg = (arguments*) malloc(t * sizeof(arguments));

	for (i = 0; i < t; i++) {
		arg[i].i = i;
		arg[i].n = n;
		pthread_create(&threads[i], NULL, (void*)generateNumbers, &arg[i]);
	}

	for (i = 0; i < t; i++) {
		pthread_join(threads[i], NULL);
	}

	clock_t end = clock();
	double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
	printf("All threads finished in %.2f s\n", time_spent);

	free(arg);
}

void generateNumbers(void* args) {
	clock_t begin = clock();

	arguments* a = (arguments *) args;
	int i;
	printf("Thread %d started.\n", a->i);
	for (i = 0; i < a->n; i++) {
		srand(time(NULL));
	}

	clock_t end = clock();
	double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
	printf("Thread %d finished in %.2f s\n",a->i, time_spent);
}

