// Author: APD team, except where source was noted

#include "helpers.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#define CONTOUR_CONFIG_COUNT    16
#define FILENAME_MAX_SIZE       50
#define STEP                    8
#define SIGMA                   200
#define RESCALE_X               2048
#define RESCALE_Y               2048

#define CLAMP(v, min, max) if(v < min) { v = min; } else if(v > max) { v = max; }

#define min(a, b) a < b ? a : b

typedef struct {
	int id;
	ppm_image* image;
	ppm_image* scaled_image;
	ppm_image** contour_map;
	unsigned char** grid;
	int step_x;
	int step_y;
	int num_threads;
	pthread_barrier_t* barrier;
} ThreadData;

// Creates a map between the binary configuration (e.g. 0110_2) and the corresponding pixels
// that need to be set on the output image. An array is used for this map since the keys are
// binary numbers in 0-15. Contour images are located in the './contours' directory.
ppm_image **init_contour_map() {
	ppm_image **map = (ppm_image **)malloc(CONTOUR_CONFIG_COUNT * sizeof(ppm_image *));
	if (!map) {
		fprintf(stderr, "Unable to allocate memory\n");
		exit(1);
	}

	for (int i = 0; i < CONTOUR_CONFIG_COUNT; i++) {
		char filename[FILENAME_MAX_SIZE];
		sprintf(filename, "./contours/%d.ppm", i);
		map[i] = read_ppm(filename);
	}

	return map;
}

// Updates a particular section of an image with the corresponding contour pixels.
// Used to create the complete contour image.
void update_image(ppm_image *image, ppm_image *contour, int x, int y) {
	for (int i = 0; i < contour->x; i++) {
		for (int j = 0; j < contour->y; j++) {
			int contour_pixel_index = contour->x * i + j;
			int image_pixel_index = (x + i) * image->y + y + j;

			image->data[image_pixel_index].red = contour->data[contour_pixel_index].red;
			image->data[image_pixel_index].green = contour->data[contour_pixel_index].green;
			image->data[image_pixel_index].blue = contour->data[contour_pixel_index].blue;
		}
	}
}

// Corresponds to step 1 of the marching squares algorithm, which focuses on sampling the image.
// Builds a p x q grid of points with values which can be either 0 or 1, depending on how the
// pixel values compare to the `sigma` reference value. The points are taken at equal distances
// in the original image, based on the `step_x` and `step_y` arguments.
unsigned char **sample_grid(ppm_image *image, int step_x, int step_y,
							unsigned char sigma, int start, int end,
							unsigned char **grid) {
	int p = image->x / step_x;
	int q = image->y / step_y;

	for (int i = start; i < end; i++) {
		for (int j = 0; j < q; j++) {
			ppm_pixel curr_pixel = image->data[i * step_x * image->y + j * step_y];

			unsigned char curr_color = (curr_pixel.red + curr_pixel.green + curr_pixel.blue) / 3;

			if (curr_color > sigma) {
				grid[i][j] = 0;
			} else {
				grid[i][j] = 1;
			}
		}
	}
	grid[p][q] = 0;

	// last sample points have no neighbors below / to the right, so we use pixels on the
	// last row / column of the input image for them
	for (int i = start; i < end; i++) {
		ppm_pixel curr_pixel = image->data[i * step_x * image->y + image->x - 1];

		unsigned char curr_color = (curr_pixel.red + curr_pixel.green + curr_pixel.blue) / 3;

		if (curr_color > sigma) {
			grid[i][q] = 0;
		} else {
			grid[i][q] = 1;
		}
	}

	for (int j = 0; j < q; j++) {
		ppm_pixel curr_pixel = image->data[(image->x - 1) * image->y + j * step_y];

		unsigned char curr_color = (curr_pixel.red + curr_pixel.green + curr_pixel.blue) / 3;

		if (curr_color > sigma) {
			grid[p][j] = 0;
		} else {
			grid[p][j] = 1;
		}
	}

	return grid;
}

// Corresponds to step 2 of the marching squares algorithm, which focuses on identifying the
// type of contour which corresponds to each subgrid. It determines the binary value of each
// sample fragment of the original image and replaces the pixels in the original image with
// the pixels of the corresponding contour image accordingly.
void march(ppm_image *image, unsigned char **grid, ppm_image **contour_map,
		   int step_x, int step_y, int start, int end) {
	int q = image->y / step_y;

	for (int i = start; i < end; i++) {
		for (int j = 0; j < q; j++) {
			unsigned char k = 8 * grid[i][j] + 4 * grid[i][j + 1] + 2 * grid[i + 1][j + 1] + 1 * grid[i + 1][j];
			update_image(image, contour_map[k], i * step_x, j * step_y);
		}
	}
}

// Calls `free` method on the utilized resources.
void free_resources(ppm_image *image, ppm_image **contour_map, unsigned char **grid, int step_x) {
	for (int i = 0; i < CONTOUR_CONFIG_COUNT; i++) {
		free(contour_map[i]->data);
		free(contour_map[i]);
	}
	free(contour_map);

	for (int i = 0; i <= image->x / step_x; i++) {
		free(grid[i]);
	}
	free(grid);

	free(image->data);
	free(image);
}

ppm_image *rescale_image(ppm_image *image, ppm_image *scaled_image, int thread_id, int num_threads) {
	uint8_t sample[3];

	// we only rescale downwards
	if (image->x <= RESCALE_X && image->y <= RESCALE_Y) {
		return image;
	}

	int start = thread_id * (double)scaled_image->x / num_threads;
    int end = min((thread_id + 1) * (double)scaled_image->x / num_threads, scaled_image->x);

	// use bicubic interpolation for scaling
	for (int i = start; i < end; i++) {
		for (int j = 0; j < scaled_image->y; j++) {
			float u = (float)i / (float)(scaled_image->x - 1);
			float v = (float)j / (float)(scaled_image->y - 1);
			sample_bicubic(image, u, v, sample);

			scaled_image->data[i * scaled_image->y + j].red = sample[0];
			scaled_image->data[i * scaled_image->y + j].green = sample[1];
			scaled_image->data[i * scaled_image->y + j].blue = sample[2];
		}
	}

	return scaled_image;
}

void* marching_squares(void* arg) {
	ThreadData* data = (ThreadData*)arg;
	int num_threads = data->num_threads;

	data->scaled_image = rescale_image(data->image, data->scaled_image, data->id, num_threads);

	pthread_barrier_wait(data->barrier);

	int p = data->image->x / data->step_x;
	int start = data->id * (double)p / num_threads;
	int end = min((data->id + 1) * (double)p / num_threads, p);

	data->grid = sample_grid(data->scaled_image, data->step_x, data->step_y, SIGMA, start, end, data->grid);

	pthread_barrier_wait(data->barrier);

	march(data->scaled_image, data->grid, data->contour_map, data->step_x,
		  data->step_y, start, end);

	return NULL;
}

int main(int argc, char *argv[]) {
	if (argc < 4) {
		fprintf(stderr, "Usage: ./tema1 <in_file> <out_file> <P>\n");
		return 1;
	}

	ppm_image *image = read_ppm(argv[1]);
	int step_x = STEP;
	int step_y = STEP;

	ppm_image **contour_map = init_contour_map();

	int num_threads = *argv[3] - 48;
	pthread_t threads[num_threads];
	ThreadData thread_data[num_threads];

	pthread_barrier_t barrier;
	int r = pthread_barrier_init(&barrier, NULL, num_threads);
	if (r) {
		printf("The barrier cannot be initialized.\n");
	}

	// alloc memory for image
	ppm_image *scaled_image = (ppm_image *)malloc(sizeof(ppm_image));
	if (!scaled_image) {
		fprintf(stderr, "Unable to allocate memory\n");
		exit(1);
	}

	scaled_image->x = RESCALE_X;
	scaled_image->y = RESCALE_Y;

	scaled_image->data = (ppm_pixel *)malloc(scaled_image->x * scaled_image->y * sizeof(ppm_pixel));
	if (!scaled_image) {
		fprintf(stderr, "Unable to allocate memory\n");
		exit(1);
	}

	int p = image->x / step_x;
	int q = image->y / step_y;

	unsigned char **grid = (unsigned char **)malloc((p + 1) * sizeof(unsigned char*));
	if (!grid) {
		fprintf(stderr, "Unable to allocate memory\n");
		exit(1);
	}

	for (int i = 0; i <= p; i++) {
		grid[i] = (unsigned char *)malloc((q + 1) * sizeof(unsigned char));
		if (!grid[i]) {
			fprintf(stderr, "Unable to allocate memory\n");
			exit(1);
		}
	}

	for (int i = 0; i < num_threads; i++) {
		thread_data[i].id = i;
		thread_data[i].image = image;
		thread_data[i].scaled_image = scaled_image;
		thread_data[i].grid = grid;
		thread_data[i].step_x = step_x;
		thread_data[i].step_y = step_y;
		thread_data[i].num_threads = num_threads;
		thread_data[i].contour_map = contour_map;
		thread_data[i].barrier = &barrier;

		pthread_create(&threads[i], NULL, marching_squares, &thread_data[i]);
	}

	for (int i = 0; i < num_threads; i++) {
		pthread_join(threads[i], NULL);
	}

	// free(image);
	// free(image->data);

	r = pthread_barrier_destroy(&barrier);
	if (r) {
		printf("The barrier cannot be destroyed.\n");
	}

	// 4. Write output
	write_ppm(thread_data[num_threads-1].scaled_image, argv[2]);

	// free_resources(scaled_image, contour_map, grid, step_x);

	return 0;
}
