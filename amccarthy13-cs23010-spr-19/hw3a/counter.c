//
// Created by amccarthy13 on 5/12/19.
//

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "utils/stopwatch.h"
#include "clhlock.h"
#include "taslock.h"
#include "alock.h"

volatile int aLockTail = 0;
volatile int *aLockFlag = 0;
volatile qnode *clhLockTail;

volatile double counter = 0;
volatile int limit = 0;

pthread_mutex_t lock;

typedef struct targs {
    float time;
    int lockType;
    int nthreads;
    int tid;
} targs;

void *parallelTimed(void *args) {
    targs *arg = (targs *) args;
    int lockType = arg->lockType;
    float time = arg->time;
    int slot;
    int nthreads;
    int padsize;
    int localCounter = 0;
    qnode *node;

    StopWatch_t stopWatch;

    if (lockType == 0) {
        startTimer(&stopWatch);
        stopTimer(&stopWatch);
        while (getElapsedTime(&stopWatch) <= time) {
            pthread_mutex_lock(&lock);
            counter++;
            pthread_mutex_unlock(&lock);
            localCounter++;
            stopTimer(&stopWatch);
        }
        //printf("thread %d incremented %d times\n", arg->tid, localCounter);
        pthread_exit(NULL);
    } else if (lockType == 1) {
        startTimer(&stopWatch);
        stopTimer(&stopWatch);
        while (getElapsedTime(&stopWatch) <= time) {
            taslock_lock();
            counter++;
            taslock_unlock();
            localCounter++;
            stopTimer(&stopWatch);
        }
        //printf("thread %d incremented %d times\n", arg->tid, localCounter);
        pthread_exit(NULL);
    } else if (lockType == 2) {
        nthreads = arg->nthreads;
        padsize = nthreads * 8;

        startTimer(&stopWatch);
        stopTimer(&stopWatch);
        while (getElapsedTime(&stopWatch) <= time) {
            alock_lock(padsize, &slot);
            counter++;
            alock_unlock(padsize, slot);
            localCounter++;
            stopTimer(&stopWatch);
        }
        //printf("thread %d incremented %d times\n", arg->tid, localCounter);
        pthread_exit(NULL);
    } else {
        node = (qnode *) malloc(sizeof(qnode));
        node->id = arg->tid;
        node->pred = NULL;

        startTimer(&stopWatch);
        stopTimer(&stopWatch);
        while (getElapsedTime(&stopWatch) <= time) {
            clh_lock(&node);
            counter++;
            clh_unlock(&node);
            localCounter++;
            stopTimer(&stopWatch);
        }
        //printf("thread %d incremented %d times\n", arg->tid, localCounter);
        free(node);
        pthread_exit(NULL);
    }
}

void *parallel(void *arg) {
    targs *args = (targs *) arg;
    int lockType = args->lockType;
    int slot;
    int nthreads = args->nthreads;
    int padsize;
    int localCounter = 0;
    qnode *node;

    if (lockType == 0) {
        while (1) {
            pthread_mutex_lock(&lock);
            if (counter < limit) {
                counter++;
            } else {
                pthread_mutex_unlock(&lock);
                //printf("thread %d incremented %d times\n", args->tid, localCounter);
                pthread_exit(NULL);
            }
            pthread_mutex_unlock(&lock);
            localCounter++;
        }
    } else if (lockType == 1) {
        while (1) {
            taslock_lock();
            if (counter < limit) {
                counter++;
            } else {
                taslock_unlock();
                //printf("thread %d incremented %d times\n", args->tid, localCounter);
                pthread_exit(NULL);
            }
            taslock_unlock();
            localCounter++;
        }
    } else if (lockType == 2) {
        padsize = nthreads * 8;

        while (1) {
            alock_lock(padsize, &slot);
            if (counter < limit) {
                counter++;
            } else {
                alock_unlock(padsize, slot);
                //printf("thread %d incremented %d times\n", args->tid, localCounter);
                pthread_exit(NULL);
            }
            alock_unlock(padsize, slot);
            localCounter++;
        }
    } else {
        node = (qnode *) malloc(sizeof(qnode));
        node->id = args->tid;
        node->pred = NULL;

        while (1) {
            clh_lock(&node);
            if (counter < limit) {
                counter++;
            } else {
                clh_unlock(&node);
                free(node);
                //printf("thread %d incremented %d times\n", args->tid, localCounter);
                pthread_exit(NULL);
            }
            clh_unlock(&node);
            localCounter++;
        }
    }
}


void serialTimed(float time) {
    StopWatch_t stopWatch;

    startTimer(&stopWatch);
    stopTimer(&stopWatch);

    while (getElapsedTime(&stopWatch) <= time) {
        counter++;
        stopTimer(&stopWatch);
    }

}

void serial() {
    while (counter < limit) {
        counter++;
    }
}

int main(int argc, char **argv) {

    StopWatch_t stopWatch;

    double serialTime = 0;
    char *type;
    int i, j, z, m;

    pthread_t *testThreads;
    testThreads = (pthread_t *) malloc((14) * sizeof(pthread_t));
    targs testArgs[14];

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    //testing code
    if (strcmp(argv[1], "test") == 0) {
        if (strcmp(argv[2], "0") == 0) {

            int bigs[5] = {1000, 50000, 700000, 100000000, 500000000};

            targs arg;

            arg.time = 0;
            arg.nthreads = 1;
            arg.tid = 1;

            pthread_t pthread;


            for (i = 0; i < 5; i++) {
                counter = 0;
                limit = bigs[i];
                startTimer(&stopWatch);
                serial();
                stopTimer(&stopWatch);
                serialTime = getElapsedTime(&stopWatch);

                for (j = 0; j < 4; j++) {
                    counter = 0;

                    if (j == 2) {
                        aLockFlag = (int *) malloc(sizeof(int) * 4);
                        aLockFlag[0] = 1;
                    }

                    if (j == 0) {
                        pthread_mutex_init(&lock, NULL);
                    }

                    arg.lockType = j;
                    if (j == 3) {
                        clhLockTail = (qnode *) malloc(sizeof(qnode));
                        clhLockTail->locked = 0;
                        clhLockTail->id = 99999;
                        clhLockTail->pred = NULL;
                    }

                    startTimer(&stopWatch);

                    if (pthread_create(&pthread, &attr, parallel, &arg)) {
                        printf("Failed to create threads");
                        exit(-1);
                    }

                    if (pthread_join(pthread, NULL)) {
                        printf("Failed to join thread");
                        exit(-1);
                    }

                    stopTimer(&stopWatch);

                    if (j == 0)
                        type = "mutex";
                    else if (j == 1)
                        type = "tas";
                    else if (j == 2)
                        type = "array";
                    else
                        type = "clh";

                    printf("ratio for %s lock with BIG set to %d is %f\n", type, bigs[i],
                           (limit/getElapsedTime(&stopWatch)) / (limit/serialTime));

                }
            }
        }
        else if (strcmp(argv[2], "1") == 0) {

            counter = 0;
            int nums[5] = {1, 2, 4, 8, 14};

            limit = 50000000;

            startTimer(&stopWatch);
            serial();
            stopTimer(&stopWatch);

            serialTime = getElapsedTime(&stopWatch);

            for (i = 0; i < 5; i++) {
                for (j = 0; j < 4; j++) {
                    if (j == 0)
                        type = "mutex";
                    else if (j == 1)
                        type = "tas";
                    else if (j == 2)
                        type = "array";
                    else
                        type = "clh";

                    counter = 0;

                    if (j == 2) {
                        aLockFlag = (int *) malloc(sizeof(int) * 4);
                        aLockFlag[0] = 1;
                    }

                    if (j == 0) {
                        pthread_mutex_init(&lock, NULL);
                    }

                    for (z = 0; z < nums[i]; z++) {
                        testArgs[z].lockType = j;
                        testArgs[z].time = 0;
                        testArgs[z].nthreads = nums[i];
                        testArgs[z].tid = z;
                        if (j == 3) {
                            clhLockTail = (qnode *) malloc(sizeof(qnode));
                            clhLockTail->locked = 0;
                            clhLockTail->id = 99999;
                            clhLockTail->pred = NULL;
                        }
                    }
                    startTimer(&stopWatch);

                    for (z = 0; z < nums[i]; z++) {
                        if (pthread_create(&testThreads[z], &attr, parallel, &testArgs[z])) {
                            printf("Failed to create threads");
                            exit(-1);
                        }
                    }

                    for (z = 0; z < nums[i]; z++) {
                        if (pthread_join(testThreads[z], NULL)) {
                            printf("Failed to join thread");
                            exit(-1);
                        }
                    }

                    stopTimer(&stopWatch);

                    printf("ratio for %s lock with %d threads is %f\n", type, nums[i],
                           (limit/getElapsedTime(&stopWatch)) / (limit/serialTime));

                    if (j == 2) {
                        free((void *) aLockFlag);
                    }
                    if (j == 3) {
                        free((void *) clhLockTail);
                    }

                }
            }
        }

        else {

            int timedNums[2] = {10};
            float times[3] = {1000, 5000, 10000};

            for (m = 0; m < 3; m++) {
                counter = 0;
                serialTimed(times[m]);
                printf("increments for serial with %f secs is %f\n", times[m] / 1000, counter);
                for (i = 0; i < 1; i++) {
                    for (j = 0; j < 4; j++) {
                        if (j == 0)
                            type = "mutex";
                        else if (j == 1)
                            type = "tas";
                        else if (j == 2)
                            type = "array";
                        else
                            type = "clh";

                        counter = 0;

                        if (j == 2) {
                            aLockFlag = (int *) malloc(sizeof(int) * 4);
                            aLockFlag[0] = 1;
                        }

                        if (j == 0) {
                            pthread_mutex_init(&lock, NULL);
                        }

                        for (z = 0; z < timedNums[i]; z++) {
                            testArgs[z].lockType = j;
                            testArgs[z].time = times[m];
                            testArgs[z].nthreads = timedNums[i];
                            testArgs[z].tid = z;
                            if (j == 3) {
                                clhLockTail = (qnode *) malloc(sizeof(qnode));
                                clhLockTail->locked = 0;
                                clhLockTail->id = 99999;
                                clhLockTail->pred = NULL;
                            }
                        }

                        for (z = 0; z < timedNums[i]; z++) {
                            if (pthread_create(&testThreads[z], &attr, parallelTimed, &testArgs[z])) {
                                printf("Failed to create threads");
                                exit(-1);
                            }
                        }

                        for (z = 0; z < timedNums[i]; z++) {
                            if (pthread_join(testThreads[z], NULL)) {
                                printf("Failed to join thread");
                                exit(-1);
                            }
                        }

                        printf("increments for %s lock with %d threads with %f secs is %f\n", type, timedNums[i],
                               times[m] / 1000, counter);

                        if (j == 2) {
                            free((void *) aLockFlag);
                        }
                        if (j == 3) {
                            free((void *) clhLockTail);
                        }
                    }
                }
            }
        }

        pthread_attr_destroy(&attr);

        return 0;
    }

    pthread_t *threads;
    int nthreads = 0;
    int isparallel = 0;
    int lockType = 0;
    bool threadFlag = true;
    bool limitFlag = true;
    bool typeFlag = true;


    for (i = 0; i < (argc - 1); i++) {
        if (strcmp(argv[i], "-n") == 0) {
            nthreads = atoi(argv[i + 1]);
            threadFlag = false;
        }

        if (strcmp(argv[i], "-p") == 0) {
            isparallel = 1;
        }

        if (strcmp(argv[i], "-l") == 0) {
            limit = atoi(argv[i + 1]);
            limitFlag = false;
        }

        if (strcmp(argv[i], "-t") == 0) {
            lockType = atoi(argv[i + 1]);
            typeFlag = false;
        }
    }

    if (isparallel == 1) {
        if (threadFlag || limitFlag || typeFlag) {
            printf("Not all required parameters provided");
            return 1;
        }
    } else {
        if (limitFlag || typeFlag) {
            printf("Not all required parameters provided");
            return 1;
        }
    }

    if (lockType > 3 || lockType < 0) {
        printf("Invalid lock type.  Must be 0 for pthread, 1 for TAS lock, 2 for Array lock, or 3 for CLH lock");
        return 1;
    }

    if (!isparallel) {
        startTimer(&stopWatch);
        serial();
        stopTimer(&stopWatch);
        printf("serial counted to %d in %f milliseconds\n", limit, getElapsedTime(&stopWatch));
    } else {
        threads = (pthread_t *) malloc((nthreads) * sizeof(pthread_t));
        targs args[nthreads];

        if (lockType == 2) {
            aLockFlag = (int *) malloc(sizeof(int) * nthreads * 4);
            aLockFlag[0] = 1;
        }

        if (lockType == 0) {
            pthread_mutex_init(&lock, NULL);
        }

        for (i = 0; i < nthreads; i++) {
            args[i].lockType = lockType;
            args[i].time = 0;
            args[i].nthreads = nthreads;
            args[i].tid = i;
            if (lockType == 3) {
                clhLockTail = (qnode *) malloc(sizeof(qnode));
                clhLockTail->locked = 0;
                clhLockTail->id = 99999;
                clhLockTail->pred = NULL;
            }
        }

        startTimer(&stopWatch);
        for (i = 0; i < nthreads; i++) {
            if (pthread_create(&threads[i], &attr, parallel, &args[i])) {
                printf("Failed to create threads");
                exit(-1);
            }
        }

        pthread_attr_destroy(&attr);
        for (i = 0; i < nthreads; i++) {
            if (pthread_join(threads[i], NULL)) {
                printf("Failed to join thread");
                exit(-1);
            }
        }
        stopTimer(&stopWatch);
        printf("Parralel counted to %f in %f milliseconds", counter, getElapsedTime(&stopWatch));

        if (lockType == 2) {
            free((void *) aLockFlag);
        }
        if (lockType == 3) {
            free((void *) clhLockTail);
        }
    }
    return 0;
}
