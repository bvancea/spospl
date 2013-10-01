#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <stdlib.h>

void *generateNumbers(void* n);

int main(int argc, char **argv) {

	int t;
	int n;

	if (argc < 2) {
		t = 2;
		n = 10000000;
	} else {
		t = (int) argv[1];
		n = (int) argv[2];
	}

	pthread_t* threads = malloc(t * sizeof(pthread_t));
	int i;
	printf("All threads starting..\n");
	for (i = 0; i < t; i++) {
		pthread_create(&threads[i], NULL, generateNumbers, &n);
	}

	for (i = 0; i < t; i++) {
		pthread_join(threads[i], NULL);
	}
	printf("All threads finished\n");

	return 0;
}

void *generateNumbers(void* n) {
	clock_t begin, end;
	double time_spent;

	begin = clock();

	int i;
	int* nr = (int *) n;
	printf("Thread started %d\n", *nr);
	for (i = 0; i < *nr; i++) {
		srand(time(NULL));
	}

	end = clock();

	time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
	printf("Thread finished in %f\n",time_spent);
}

