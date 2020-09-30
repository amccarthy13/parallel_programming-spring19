#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <pthread.h>
#include "stopwatch.c"
#include "stopwatch.h"

#define MAX 10000000
int A[MAX];
int B[MAX];
int n;
int num_thread;
pthread_barrier_t barr;

int loader() {
    int i, j, rand_int;
    for (i = 0; i < n; i++) {
        for (j=0; j < n; j++) {
            if (i == j) {
                A[n*i+j] = 0;
                B[n*i+j] = 0;
            } else {
                rand_int = (rand() % 999) + 1;
                A[n*i+j] = rand_int;
                B[n*i+j] = rand_int;
            }
        }
    }

    return 1;
}

void* parallel(void* arg) {
    int i,j,k;
    int rank;
    rank = (int) (__intptr_t) arg;

    int start = rank * n / num_thread;
    int end   = (rank + 1) * n / num_thread;

    for(k = 0; k < n; k++)
    {
        int rc = pthread_barrier_wait(&barr);
        if(rc != 0 && rc != PTHREAD_BARRIER_SERIAL_THREAD)
        {
            printf("barrier failed\n");
            exit(-1);
        }
        for(i = start; i < end ; i++)
        {
            for(j = 0; j < n; j++)
            {
                if (A[i*n+k]+A[k*n+j]<A[i*n+j])
                    A[i*n+j]=A[i*n+k]+A[k*n+j];
            }
        }
    }
    return 0;
}

void serial() {
    int i,j,k;

    for(k = 0; k < n; k++)
    {
        for(i = 0; i < n ; i++)
        {
            for(j = 0; j < n; j++)
            {
                if (B[i*n+k]+B[k*n+j]<B[i*n+j])
                    B[i*n+j]=B[i*n+k]+B[k*n+j];
            }
        }
    }
}


int main(int argc, char* argv[]) {
    int vertices [6] = {16, 32, 64, 128, 512, 1024};
    int threads [5] = {2, 4, 16, 32, 64};
    pthread_t* thread;
    int i, j;
    int x, y;
    int parallelTime, serialTime;
    StopWatch_t stopWatch;

    num_thread = 1;
    if (pthread_barrier_init(&barr, NULL, num_thread)) {
        printf("Could not create a barrier\n");
        return -1;
    }
    for (x = 0; x<6;x++) {
        n = vertices[x];
        loader();

        startTimer(&stopWatch);
        thread = (pthread_t *) malloc(num_thread * sizeof(pthread_t));
        for (i = 0; i < num_thread; ++i) {
            if (pthread_create(&thread[i], NULL, &parallel, (void *) (__intptr_t) i)) {
                printf("Could not create a thread %d\n", i);
                free(thread);
                return -1;
            }
        }
        for (i = 0; i < num_thread; i++)
            pthread_join(thread[i], NULL);
        free(thread);

        stopTimer(&stopWatch);
        parallelTime = getElapsedTime(&stopWatch);

        startTimer(&stopWatch);

        serial();

        stopTimer(&stopWatch);
        serialTime = getElapsedTime(&stopWatch);

        for (i = 0; i < n; i++) {
            for (j = 0; j < n; j++) {
                if (A[n*i+j] != B[n*i+j]) {
                    printf("POSITION %d, %d IS INCORRECT!\n", i, j);
                    return -1;
                }
            }
        }

        printf("overhead with %d vertices: serial %d, parallel %d\n", vertices[x], serialTime, parallelTime);
    }

    for (y = 0; y < 5; y++) {
        num_thread = threads[y];
        if (pthread_barrier_init(&barr, NULL, num_thread)) {
            printf("Could not create a barrier\n");
            return -1;
        }
        for (x = 0; x < 6; x++) {
            n = vertices[x];
            loader();

            startTimer(&stopWatch);
            thread = (pthread_t *) malloc(num_thread * sizeof(pthread_t));
            for (i = 0; i < num_thread; ++i) {
                if (pthread_create(&thread[i], NULL, &parallel, (void *) (__intptr_t) i)) {
                    printf("Could not create a thread %d\n", i);
                    free(thread);
                    return -1;
                }
            }
            for (i = 0; i < num_thread; i++)
                pthread_join(thread[i], NULL);
            free(thread);

            stopTimer(&stopWatch);
            parallelTime = getElapsedTime(&stopWatch);

            startTimer(&stopWatch);

            serial();

            stopTimer(&stopWatch);
            serialTime = getElapsedTime(&stopWatch);

            for (i = 0; i < n; i++) {
                for (j = 0; j < n; j++) {
                    if (A[n*i+j] != B[n*i+j]) {
                        printf("POSITION %d, %d IS INCORRECT!\n", i, j);
                        return -1;
                    }
                }
            }

            printf("For %d vertices and %d threads, serial time was %d and parallel time was %d\n", n, num_thread, serialTime, parallelTime);
        }
    }

    return 0;
}

