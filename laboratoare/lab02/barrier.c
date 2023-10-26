#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
// #include "pthread_barrier_mac.h"
#define NUM_THREADS 2

pthread_barrier_t barrier;

// TODO: de folosit bariera in aceasta functie
void *f(void *arg)
{
	int thread_id = *(int *)arg;

	if (thread_id == 1) {
		printf("1\n");
	}

	// Make sure that both of the threads get to this point
	pthread_barrier_wait(&barrier);

	if (thread_id == 0) {
		printf("2\n");
	}

	pthread_exit(NULL);
}

int main(int argc, char **argv)
{
	int i, r;
	void *status;
	pthread_t threads[NUM_THREADS];
	int arguments[NUM_THREADS];

	r = pthread_barrier_init(&barrier, NULL, 2);
	if (r) {
		printf("The barrier cannot be initialized.\n");
	}

	for (i = 0; i < NUM_THREADS; i++) {
		arguments[i] = i;
		r = pthread_create(&threads[i], NULL, f, &arguments[i]);

		if (r) {
			printf("Eroare la crearea thread-ului %d\n", i);
			exit(-1);
		}
	}

	for (i = 0; i < NUM_THREADS; i++) {
		r = pthread_join(threads[i], &status);

		if (r) {
			printf("Eroare la asteptarea thread-ului %d\n", i);
			exit(-1);
		}
	}

	r = pthread_barrier_destroy(&barrier);
	if (r) {
		printf("The barrier cannot be destroyed.\n");
	}

	return 0;
}
