#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void *f(void *arg) {
	long id = *(long *)arg;
	printf("Apelul lui F din thread-ul %ld!\n", id);
	pthread_exit(NULL);
}

void *g(void *arg) {
	long id = *(long *)arg;
  	printf("Apelul lui G din thread-ul %ld!\n", id);
  	pthread_exit(NULL);
}

void *f100(void *arg) {
	long thread_id = *(long *)arg;
	
	for (int i = 0; i < 100; i++) {
		printf("Hello World (%d) din thread-ul %ld!\n", i, thread_id);
	}

	pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
	long cores = sysconf(_SC_NPROCESSORS_CONF);

	pthread_t threads[cores];
	int r;
	long id;
	void *status;
	long ids[cores];

	// exercitiul 1,2
/*
	for (id = 0; id < cores; id++) {
		ids[id] = id;
		r = pthread_create(&threads[id], NULL, f, &ids[id]);

		if (r) {
		printf("Eroare la crearea thread-ului %ld\n", id);
		exit(-1);
		}
	}
*/
	// exercitiul 3
/*
	for (id = 0; id < cores; id++) {
		ids[id] = id;
		r = pthread_create(&threads[id], NULL, f100, &ids[id]);

		if (r) {
		printf("Eroare la crearea thread-ului %ld\n", id);
		exit(-1);
		}
	}
*/

	// for (id = 0; id < cores; id++) {
	// 	r = pthread_join(threads[id], &status);

	// 	if (r) {
	// 	printf("Eroare la asteptarea thread-ului %ld\n", id);
	// 	exit(-1);
	// 	}
	// }

	// exercitiul 4
	pthread_t thread1, thread2;
	ids[0] = 0;
	r = pthread_create(&thread1, NULL, f, &ids[0]);
	if (r) {
		printf("Eroare la asteptarea thread-ului %ld\n", id);
		exit(-1);
	}

	ids[1] = 1;
	r = pthread_create(&thread2, NULL, g, &ids[1]);
	if (r) {
		printf("Eroare la asteptarea thread-ului %ld\n", id);
		exit(-1);
	}

	r = pthread_join(thread1, &status);
	if (r) {
		printf("Eroare la asteptarea thread-ului 0\n");
		exit(-1);
	}
	
	r = pthread_join(thread2, &status);
	if (r) {
		printf("Eroare la asteptarea thread-ului 1\n");
		exit(-1);
	}


  return 0;
}
