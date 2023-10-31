*Copyright (C) 2023 Cismaru Diana-Iuliana (331CA - 2023/2024)*

# Parallel Contour Curve Drawing of Marching Squares Algorithm
# Homework #1 Parallel and Distributed Algorithms

## Table of Contents
1. [Description of the Project](#1-description-of-the-project)
2. [Thread Arguments](#2-thread-arguments)
3. [Synchronization](#3-synchronization)

## 1. Description of the Project

The project involves parallelizing an algorithm that has already been implemented
sequentially, namely the Marching Squares algorithm. It consists of processing
images by detecting transitions between background and foreground regions, and
generating a contour that approximates the shape of these regions. I have chosen
to parallelize this implementation using the `pthread` library.

Considering that most operations are performed on matrices, my parallelization
method involves dividing the arrays into **num_threads** threads that will
perform parallel computations. The parallel sections are delimited by using the
`start` and `end` variables, where:

    start = thread_id * (double)array_size / num_threads
    end = min((thread_id + 1) * (double)array_size / num_threads, array_size)

## 2. Thread Arguments

Each thread passes the necessary data to the `parallel_marching_squares`
function, stored in an array of structures called `ThreadData`. The fields of 
this structure represent the arguments required by the rescale, grid, and march
functions. Additionally, I have also stored the unique **identifier** of the
current thread in that structure, as well as a pointer to a **synchronization**
**barrier**.


## 3. Synchronization
Thread synchronization is achieved using a **barrier**, whose counter is
initialized with the number of threads we create. This barrier is responsible
for synchronizing the threads when we want to create the `grid`, because we need
to ensure that the image has been scaled properly beforehand. It is also used
when we want to call the `march` function, because we must already have the grid
constructed.
