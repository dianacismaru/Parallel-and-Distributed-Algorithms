#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define min(a, b) a > b ? b : a

/*
	schelet pentru exercitiul 5
*/

int *arr;

// array size
int N;

// number of threads
int P;

void *f(void *arg) {
	long id = *(long *)arg;

	int start = id * (double)N / P;
	int end = min((id + 1) * (double)N / P, N);

	for (int i = start; i < end; i++) {
		arr[i] += 100;
	}

	pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
	if (argc < 2) {
		fprintf(stderr, "Specificati dimensiunea array-ului\n");
		exit(-1);
	}

	N = atoi(argv[1]);
	P = sysconf(_SC_NPROCESSORS_CONF);

	arr = malloc(N * sizeof(int));
	for (int i = 0; i < N; i++) {
		arr[i] = i;
	}

	for (int i = 0; i < N; i++) {
		printf("%d", arr[i]);
		if (i != N - 1) {
			printf(" ");
		} else {
			printf("\n");
		}
	}

	pthread_t threads[P];
	int r;
	long id;
	void *status;
	long ids[P];
	
	for (id = 0; id < P; id++) {
		ids[id] = id;
		r = pthread_create(&threads[id], NULL, f, &ids[id]);

		if (r) {
			printf("Eroare la crearea thread-ului %ld\n", id);
			exit(-1);
		}
	}

	for (id = 0; id < P; id++) {
		r = pthread_join(threads[id], &status);

		if (r) {
			printf("Eroare la asteptarea thread-ului %ld\n", id);
			exit(-1);
		}
	}

	// Print the final array
	for (int i = 0; i < N; i++) {
		printf("%d", arr[i]);
		if (i != N - 1) {
			printf(" ");
		} else {
			printf("\n");
		}
	}

	return 0;
}
