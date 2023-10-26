#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define NUM_THREADS 2

int a = 0;
// Mutex declaration
pthread_mutex_t mutex;

// TODO: adaugati mutexul in functia de mai jos
void *f(void *arg)
{
	int r = pthread_mutex_lock(&mutex);
	if (r) {
		printf("Lock cannot be done.\n");
	}

	// Regiune critica
	a += 2;

	r = pthread_mutex_unlock(&mutex);
	if (r) {
		printf("Unlock cannot be done.\n");
	}

	pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
	int i, r;
	void *status;
	pthread_t threads[NUM_THREADS];
	int arguments[NUM_THREADS];

	// Initialize the mutex
	pthread_mutex_init(&mutex, NULL);

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

	// Clean up the mutex
	pthread_mutex_destroy(&mutex);
	printf("a = %d\n", a);

	return 0;
}
