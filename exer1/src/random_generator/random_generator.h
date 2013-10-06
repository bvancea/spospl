/*
 * random_generator.h
 */
#ifndef RANDOM_GENERATOR_H_
#define RANDOM_GENERATOR_H_

#define DEFAULT_N 100000

typedef struct argument_t {
	int n;
	int i;
} arguments;

void generateNumbers(void* n);
void createWorkerThreads(pthread_t* threads, int t, int n);

#endif /* RANDOM_GENERATOR_H_ */
