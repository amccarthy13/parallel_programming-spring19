#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <pthread.h>
#define NODE 7
#define INF 999

#define MAX 10000000
int A[MAX];
int n;
char* fname;
int num_thread;
pthread_barrier_t barr;

int loader(int* A) {
    FILE* lo;
    int i,j;
    int scan = 0;
    if ((lo=fopen(fname,"r"))==NULL) {
        printf("Error opening the input data.\n");
        return 1;
    }
    scan = fscanf(lo,"%d\n",&n);

    if (n < 2) {
        printf("Error: Matrix cannot be of size 1 x 1");
        return 1;
    }

    for (i=0;i<n;i++)
        for (j=0;j<n;j++)
            scan = fscanf(lo,"%d\t",A+n*i+j);
    fclose(lo);
    if (scan > 0) {
        
    }
    return 0;
}

int saver(int* A, int n) {
    FILE* sa;
    int i, j;
    if ((sa = fopen("output_file.txt", "w")) == NULL) {
        printf("Error creating the output data.\n");
        return 1;
    }
    fprintf(sa, "%d\n", n);
    for (i = 0; i<n;i++) {
        for (j=0;j<n;j++)
            fprintf(sa, "%d\t", A[n*i+j]);
        fprintf(sa, "\n");
    }
    fclose(sa);

    return 0;
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
                if (A[i*n+k]+A[k*n+j]<A[i*n+j])
                    A[i*n+j]=A[i*n+k]+A[k*n+j];
            }
        }
    }
}


int main(int argc, char* argv[]) {
    pthread_t* thread;
    int i;
    int load, save;

    if (argc < 2) {
        printf("No parameters given\n");
        return -1;
    } else if (argc == 2) {
        fname=argv[1];
        load = loader(A);
        if (load == 1)
            return -1;
        serial();
        save = saver(A, n);
        if (save == 1)
            return -1;
        return 0;
    } else if (argc == 3) {
        num_thread = strtol(argv[1], NULL, 10);
        fname = argv[2];
        load = loader(A);
        if (load == 1)
            return -1;

        if (pthread_barrier_init(&barr, NULL, num_thread)) {
            printf("Could not create a barrier\n");
            return -1;
        }
        for (i = 0; i < num_thread; ++i) {
            if (pthread_create(&thread[i], NULL, &parallel, (void *) (__intptr_t) i)) {
                printf("Could not create a thread %d\n", i);
                free(thread);
                return -1;
            }
        }
        for (i = 0; i < num_thread; i++)
            pthread_join(thread[i], NULL);
        save = saver(A, n);
        if (save == 1)
            return -1;
        return 0;
    } else {
        printf("Too many parameters given.  Expected file name for serial or thread count and file name for parallel.\n");
        return -1;
    }
}