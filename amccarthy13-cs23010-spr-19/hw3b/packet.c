//
// Created by amccarthy13 on 5/17/19.
//

#include "utils/fingerprint.h"
#include "utils/packetsource.h"
#include "utils/stopwatch.h"
#include "clhlock.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <pthread.h>
#include <math.h>
#include <unistd.h>
#include <stdbool.h>

#define MAX 1000

typedef struct queue {
    volatile int head, tail;
    volatile Packet_t pqueue[100];
} queue;

queue *queues;
volatile qnode *clhLockTail;
pthread_mutex_t lock;
volatile int *endFlags;
int depth;

struct queueArgs {
    int sourceNum;
    double processed;
    int lockType;
    int numSources;
};

int enqueue(volatile Packet_t *packet, int source) {
    if ((queues[source].tail - queues[source].head) == depth) {
        return 1;
    }
    queues[source].pqueue[queues[source].tail % depth] = *packet;
    queues[source].tail++;
    return 0;
}

volatile Packet_t *dequeue(int source) {
    if ((queues[source].tail - queues[source].head) == 0) {
        return NULL;
    }
    volatile Packet_t *x = &queues[source].pqueue[queues[source].head % depth];
    queues[source].head++;
    return x;
}

void *workerLockFree(void *arg) {
    struct queueArgs *args;
    args = (struct queueArgs *) arg;
    volatile Packet_t *packet;
    double processed = 0;

    while (endFlags[args->sourceNum] == 0) {
        packet = dequeue(args->sourceNum);
        if (packet == NULL) {
            continue;
        } else {
            getFingerprint(packet->iterations, packet->seed);
            processed++;
        }
    }
    args->processed = processed;
    pthread_exit(NULL);
}

void *homeQueue(void *arg) {
    struct queueArgs *args;
    args = (struct queueArgs *) arg;
    volatile Packet_t *packet;
    double processed = 0;
    qnode *node;

    node = (qnode *) malloc(sizeof(qnode));
    node->id = args->sourceNum;
    node->pred = NULL;

    while (endFlags[args->sourceNum] == 0) {
        if (args->lockType == 0) {
            pthread_mutex_lock(&lock);
            packet = dequeue(args->sourceNum);
            pthread_mutex_unlock(&lock);
        } else {
            clh_lock(&node);
            packet = dequeue(args->sourceNum);
            clh_unlock(&node);
        }
        if (packet == NULL) {
            continue;
        } else {
            getFingerprint(packet->iterations, packet->seed);
            processed++;
        }
    }
    args->processed = processed;
    pthread_exit(NULL);
}

void *awesomeQueue(void *arg) {
    struct queueArgs *args;
    args = (struct queueArgs *) arg;
    volatile Packet_t *packet;
    double processed = 0;

    int i = args->sourceNum;

    if (args->lockType == 0) {
        while (endFlags[args->sourceNum] == 0) {
            if (pthread_mutex_trylock(&lock) == 0) {
                packet = dequeue(i);
                pthread_mutex_unlock(&lock);
                if (packet == NULL) {
                    continue;
                } else {
                    getFingerprint(packet->iterations, packet->seed);
                    processed++;
                }
            }
            if (i != args->numSources - 1)
                i++;
            else
                i = 0;
        }
    } else {
        qnode *node;
        node = (qnode *) malloc(sizeof(qnode));
        node->id = args->sourceNum;
        node->pred = NULL;
        while (endFlags[args->sourceNum] == 0) {
            if (clh_tryLock(&node)) {
                packet = dequeue(i);
                clh_unlock(&node);
                if (packet == NULL) {
                    continue;
                } else {
                    getFingerprint(packet->iterations, packet->seed);
                    processed++;
                }
            }
            if (i != args->numSources - 1)
                i++;
            else
                i = 0;
        }
    }
    args->processed = processed;
    pthread_exit(NULL);
}

double serial(PacketSource_t *packetSource, int numSources, int time, int generationType, int expectedWork) {
    int i = 0;
    double processed = 0;
    volatile Packet_t *packet;
    StopWatch_t stopWatch;

    startTimer(&stopWatch);
    stopTimer(&stopWatch);
    while (getElapsedTime(&stopWatch) <= time) {
        if (generationType == 0)
            packet = getUniformPacket(packetSource, i);
        else
            packet = getExponentialPacket(packetSource, i);
        getFingerprint(packet->iterations, packet->seed);
        processed++;
        if (i == numSources - 1)
            i = 0;
        else
            i++;
        stopTimer(&stopWatch);
    }
    return processed;
}

void dispatcher(int numQueues, PacketSource_t *packetSource, int generationType, int time) {
    int i = 0;
    int j;
    volatile Packet_t *packet;
    StopWatch_t stopWatch;

    startTimer(&stopWatch);
    stopTimer(&stopWatch);
    while (getElapsedTime(&stopWatch) <= time) {
        if (generationType == 0)
            packet = getUniformPacket(packetSource, i);
        else
            packet = getExponentialPacket(packetSource, i);
        while (enqueue(packet, i) == 1) {}

        if (i == (numQueues - 1)) {
            i = 0;
        } else {
            i++;
        }
        stopTimer(&stopWatch);
    }
    for (j = 0; j < numQueues; j++) {
        endFlags[j] = 1;
    }
}

void dispatcherTest(int numQueues, PacketSource_t *packetSource, int generationType, int packetsToDo) {
    int i = 0;
    int j;
    volatile Packet_t *packet;
    int threadPackets[numQueues];
    for (j = 0; j < numQueues; j++) {
        threadPackets[j] = packetsToDo;
    }
    double packetsLeft = (numQueues * packetsToDo);

    do {
        if (threadPackets[i] > 0) {
            if (generationType == 0)
                packet = getUniformPacket(packetSource, i);
            else
                packet = getExponentialPacket(packetSource, i);
            if (enqueue(packet, i) == 0) {
                threadPackets[i]--;
                packetsLeft--;
            }
        }

        if (i == (numQueues - 1)) {
            i = 0;
        } else {
            i++;
        }
    } while (packetsLeft > 0);
    sleep(2);
    for (j = 0; j < numQueues; j++) {
        endFlags[j] = 1;
    }
}

int main(int argc, char *argv[]) {
    pthread_t *threads;
    int seed = 1;
    int i, x, y, z, m;

    if (argc < 2) {
        printf("No parameters given\n");
        return -1;
    }

    if (strcmp(argv[1], "test") == 0) {
        if (argc < 4) {
            printf("Need to provide a seed and test number for testing (in that order).\n");
            return -1;
        }

        struct queueArgs *args;
        args = malloc(27 * sizeof(struct queueArgs));
        threads = (pthread_t *) malloc(27 * sizeof(pthread_t));
        queues = (queue *) malloc(sizeof(queue) * 27);

        seed = atoi(argv[2]);
        depth = 8;

        int test = atoi(argv[3]);

        int worksSpeedup[4] = {1000, 2000, 4000, 8000};
        int counts[6] = {1, 2, 3, 7, 13, 27};

        char *generation;

        double serialProcessed = 0;
        double parallelProcessed = 0;

        int works[6] = {25, 50, 100, 200, 400, 800};
        char *type;
        double lockFreeProcessed = 0;
        double homeProcessed = 0;

        endFlags = malloc(27 * sizeof(int));
        //correctness testing
        if (test == 0) {
            for (z = 0; z < 3; z++) {
                for (y = 0; y < 2; y++) {

                    for (i = 0; i < 8; i++) {
                        queues[i].tail = 0;
                        queues[i].head = 0;
                    }

                    PacketSource_t *packetSource = createPacketSource(20, 8, seed);

                    for (i = 0; i < 8; ++i) {
                        endFlags[i] = 0;
                    }

                    for (i = 0; i < 8; ++i) {
                        args[i].sourceNum = i;
                        args[i].numSources = 8;
                        args[i].lockType = y;
                        args[i].processed = 0;
                        if (y == 1) {
                            clhLockTail = (qnode *) malloc(sizeof(qnode));
                            clhLockTail->locked = 0;
                            clhLockTail->id = 99999;
                            clhLockTail->pred = NULL;
                        }
                        if (z == 0) {
                            if (pthread_create(&threads[i], NULL, workerLockFree, &args[i])) {
                                printf("Could not create a thread %d\n", i);
                                free(threads);
                                return -1;
                            }
                        } else if (z == 1) {
                            if (pthread_create(&threads[i], NULL, homeQueue, &args[i])) {
                                printf("Could not create a thread %d\n", i);
                                free(threads);
                                return -1;
                            }
                        } else {
                            if (pthread_create(&threads[i], NULL, awesomeQueue, &args[i])) {
                                printf("Could not create a thread %d\n", i);
                                free(threads);
                                return -1;
                            }
                        }
                    }
                    dispatcherTest(8, packetSource, 0, 20);

                    for (i = 0; i < 8; i++) {
                        pthread_join(threads[i], NULL);
                    }

                    double totalProcessed = 0;
                    for (i = 0; i < 8; i++) {
                        totalProcessed += args[i].processed;
                    }

                    if (y == 1) {
                        free((void *) clhLockTail);
                    }

                    printf("dispatcher queued %d packets and processed %f.\n", (20 * 8), totalProcessed);


                }
            }
        }

            //overhead testing
        else if (test == 1) {
            for (z = 0; z < 6; z++) {
                printf("\n");
                for (y = 0; y < 2; y++) {
                    if (y == 0)
                        type = "mutex";
                    else
                        type = "clh";

                    queues[0].tail = 0;
                    queues[0].head = 0;

                    PacketSource_t *packetSource = createPacketSource(works[z], 1, seed);

                    endFlags[0] = 0;

                    args[0].sourceNum = 0;
                    args[0].numSources = 1;
                    args[0].lockType = y;
                    args[0].processed = 0;

                    if (y == 1) {
                        clhLockTail = (qnode *) malloc(sizeof(qnode));
                        clhLockTail->locked = 0;
                        clhLockTail->id = 99999;
                        clhLockTail->pred = NULL;
                    }

                    if (pthread_create(&threads[0], NULL, workerLockFree, &args[0])) {
                        printf("Could not create a thread %d\n", 0);
                        free(threads);
                        return -1;
                    }
                    dispatcher(1, packetSource, 0, 2000);

                    pthread_join(threads[0], NULL);

                    lockFreeProcessed = 0;
                    lockFreeProcessed += args[0].processed;

                    if (y == 1) {
                        free((void *) clhLockTail);
                    }


                    queues[0].tail = 0;
                    queues[0].head = 0;

                    endFlags[0] = 0;

                    args[0].sourceNum = 0;
                    args[0].numSources = 1;
                    args[0].lockType = y;
                    args[0].processed = 0;
                    if (y == 1) {
                        clhLockTail = (qnode *) malloc(sizeof(qnode));
                        clhLockTail->locked = 0;
                        clhLockTail->id = 99999;
                        clhLockTail->pred = NULL;
                    }
                    if (pthread_create(&threads[0], NULL, homeQueue, &args[0])) {
                        printf("Could not create a thread %d\n", 0);
                        free(threads);
                        return -1;
                    }
                    dispatcher(1, packetSource, 0, 2000);

                    pthread_join(threads[0], NULL);

                    homeProcessed = 0;
                    homeProcessed += args[0].processed;

                    if (y == 1) {
                        free((void *) clhLockTail);
                    }
                    printf("Overhead ratio with w = %d and lock type = %s in %d seconds is %f\n", works[z], type, 2,
                           lockFreeProcessed / homeProcessed);
                }
            }
        }

            //speedup testing
        else if (test == 2) {
            for (m = 0; m < 2; m++) {
                if (m == 0)
                    generation = "uniform";
                else
                    generation = "exponential";
                for (z = 0; z < 4; z++) {
                    for (y = 0; y < 6; y++) {
                        printf("\n");
                        PacketSource_t *packetSource;
                        packetSource = createPacketSource(worksSpeedup[z], counts[y], seed);
                        serialProcessed = 0;
                        serialProcessed = serial(packetSource, counts[y], 2000, m, worksSpeedup[z]);


                        for (i = 0; i < counts[y]; i++) {
                            queues[i].tail = 0;
                            queues[i].head = 0;
                        }

                        for (i = 0; i < counts[y]; ++i) {
                            endFlags[i] = 0;
                        }

                        for (i = 0; i < counts[y]; ++i) {
                            args[i].sourceNum = i;
                            args[i].numSources = counts[y];
                            args[i].lockType = 0;
                            args[i].processed = 0;

                            if (pthread_create(&threads[i], NULL, workerLockFree, &args[i])) {
                                printf("Could not create a thread %d\n", i);
                                free(threads);
                                return -1;
                            }
                        }
                        dispatcher(counts[y], packetSource, m, 2000);

                        for (i = 0; i < counts[y]; i++) {
                            pthread_join(threads[i], NULL);
                        }

                        parallelProcessed = 0;
                        for (i = 0; i < counts[y]; i++) {
                            parallelProcessed += args[i].processed;
                        }


                        printf("speedup with lock free and w = %d, and n = %d, and %s packet generation in 2 seconds is %f\n",
                               worksSpeedup[z], counts[y], generation, parallelProcessed / serialProcessed);
                        for (x = 0; x < 2; x++) {
                            if (x == 0)
                                type = "mutex";
                            else
                                type = "clh";

                            for (i = 0; i < counts[y]; i++) {
                                queues[i].tail = 0;
                                queues[i].head = 0;
                            }

                            for (i = 0; i < counts[y]; ++i) {
                                endFlags[i] = 0;
                            }

                            for (i = 0; i < counts[y]; ++i) {
                                args[i].sourceNum = i;
                                args[i].numSources = counts[y];
                                args[i].lockType = x;
                                args[i].processed = 0;
                                if (x == 1) {
                                    clhLockTail = (qnode *) malloc(sizeof(qnode));
                                    clhLockTail->locked = 0;
                                    clhLockTail->id = 99999;
                                    clhLockTail->pred = NULL;
                                }
                                if (pthread_create(&threads[i], NULL, homeQueue, &args[i])) {
                                    printf("Could not create a thread %d\n", i);
                                    free(threads);
                                    return -1;
                                }
                            }
                            dispatcher(counts[y], packetSource, m, 2000);

                            for (i = 0; i < counts[y]; i++) {
                                pthread_join(threads[i], NULL);
                            }

                            parallelProcessed = 0;
                            for (i = 0; i < counts[y]; i++) {
                                parallelProcessed += args[i].processed;
                            }

                            if (y == 1) {
                                free((void *) clhLockTail);
                            }

                            printf("speedup with home queue and w = %d, and n = %d, and %s packet generation and lock type %s in 2 seconds is %f\n",
                                   worksSpeedup[z], counts[y], generation, type, parallelProcessed / serialProcessed);
                        }
                    }
                }
            }
        }

            //awesome testing
        else {
            for (m = 0; m < 2; m++) {
                if (m == 0)
                    generation = "uniform";
                else
                    generation = "exponential";
                for (z = 0; z < 4; z++) {
                    for (y = 0; y < 6; y++) {

                        printf("\n");
                        PacketSource_t *packetSource;
                        packetSource = createPacketSource(worksSpeedup[z], counts[y], seed);

                        for (i = 0; i < counts[y]; i++) {
                            queues[i].tail = 0;
                            queues[i].head = 0;
                        }

                        for (i = 0; i < counts[y]; ++i) {
                            endFlags[i] = 0;
                        }


                        for (i = 0; i < counts[y]; ++i) {
                            args[i].sourceNum = i;
                            args[i].numSources = counts[y];
                            args[i].lockType = 0;
                            args[i].processed = 0;

                            if (pthread_create(&threads[i], NULL, workerLockFree, &args[i])) {
                                printf("Could not create a thread %d\n", i);
                                free(threads);
                                return -1;
                            }
                        }

                        dispatcher(counts[y], packetSource, m, 2000);


                        for (i = 0; i < counts[y]; i++) {
                            pthread_join(threads[i], NULL);
                        }

                        lockFreeProcessed = 0;
                        for (i = 0; i < counts[y]; i++) {
                            lockFreeProcessed += args[i].processed;
                        }


                        for (x = 0; x < 2; x++) {
                            if (x == 0)
                                type = "mutex";
                            else
                                type = "clh";


                            for (i = 0; i < counts[y]; i++) {
                                queues[i].tail = 0;
                                queues[i].head = 0;

                                for (i = 0; i < counts[y]; ++i) {
                                    endFlags[i] = 0;
                                }

                                for (i = 0; i < counts[y]; ++i) {
                                    args[i].sourceNum = i;
                                    args[i].numSources = counts[y];
                                    args[i].lockType = x;
                                    args[i].processed = 0;
                                    if (x == 1) {
                                        clhLockTail = (qnode *) malloc(sizeof(qnode));
                                        clhLockTail->locked = 0;
                                        clhLockTail->id = 99999;
                                        clhLockTail->pred = NULL;
                                    }
                                    if (pthread_create(&threads[i], NULL, awesomeQueue, &args[i])) {
                                        printf("Could not create a thread %d\n", i);
                                        free(threads);
                                        return -1;
                                    }
                                }
                                dispatcher(counts[y], packetSource, m, 2000);

                                for (i = 0; i < counts[y]; i++) {
                                    pthread_join(threads[i], NULL);
                                }

                                parallelProcessed = 0;
                                for (i = 0; i < counts[y]; i++) {
                                    parallelProcessed += args[i].processed;
                                }

                                if (y == 1) {
                                    free((void *) clhLockTail);
                                }

                                printf("awesome speedup compared to lock free with and w = %d, and n = %d, and %s packet generation and lock type %s in 2 seconds is %f\n",
                                       worksSpeedup[z], counts[y], generation, type,
                                       parallelProcessed / lockFreeProcessed);
                            }
                        }
                    }
                }
            }


        }
        return 0;
    }

    bool serialFlag = true;
    int numMilliseconds = 5000;
    int numSources = 1;
    int expectedWork = 1000;
    int uniform = 0;
    int lockType = 0;
    int strategy = 0;

    bool timeFlag = true;
    bool sourceFlag = true;
    bool workFlag = true;
    bool typeFlag = true;
    bool depthFlag = true;
    bool strategyFlag = true;
    bool seedFlag = true;

    for (
            i = 0;
            i < (argc - 1); i++) {
        if (
                strcmp(argv[i],
                       "-parallel") == 0) {
            serialFlag = false;
        }

        if (
                strcmp(argv[i],
                       "-milliseconds") == 0) {
            numMilliseconds = atoi(argv[i + 1]);
            timeFlag = false;
        }

        if (
                strcmp(argv[i],
                       "-source") == 0) {
            numSources = atoi(argv[i + 1]);
            sourceFlag = false;
        }

        if (
                strcmp(argv[i],
                       "-depth") == 0) {
            depth = atoi(argv[i + 1]);
            if (depth > 100) {
                printf("depth cannot be greater than 100\n");
            }
            depthFlag = false;
        }

        if (
                strcmp(argv[i],
                       "-w") == 0) {
            expectedWork = atoi(argv[i + 1]);
            workFlag = false;
        }

        if (
                strcmp(argv[i],
                       "-exp") == 0) {
            uniform = 1;
        }

        if (
                strcmp(argv[i],
                       "-type") == 0) {
            lockType = atoi(argv[i + 1]);
            typeFlag = false;
        }

        if (
                strcmp(argv[i],
                       "-strategy") == 0) {
            strategy = atoi(argv[i + 1]);
            strategyFlag = false;
        }

        if (
                strcmp(argv[i],
                       "-seed") == 0) {
            seed = atoi(argv[i + 1]);
            seedFlag = false;
        }
    }

    if (serialFlag) {
        if (timeFlag || sourceFlag || workFlag || seedFlag) {
            printf("Not all required parameters provided: \n"
                   "need : -milliseconds -source -w -seed \n");
            return -1;
        }

        PacketSource_t *packetSource = createPacketSource(expectedWork, numSources, seed);

        printf("Number of packets processed in %d seconds is %f.\n", numMilliseconds,
               serial(packetSource, numSources, numMilliseconds, uniform, expectedWork
               ));
    } else {
        if (timeFlag || sourceFlag || workFlag || seedFlag || depthFlag || typeFlag || strategyFlag) {
            printf("Not all required parameters provided: \n"
                   "need : -milliseconds -source -depth -w -seed -type -strategy\n");
            return -1;
        }

        PacketSource_t *packetSource = createPacketSource(expectedWork, numSources, seed);
        threads = (pthread_t *) malloc(numSources * sizeof(pthread_t));

        struct queueArgs *args;
        args = malloc(numSources * sizeof(struct queueArgs));
        endFlags = malloc(numSources * sizeof(int));
        for (
                i = 0;
                i < numSources;
                ++i) {
            endFlags[i] = 0;
        }

        queues = (queue *) malloc(sizeof(queue) * numSources);

        for (
                i = 0;
                i < numSources;
                ++i) {
            args[i].
                    sourceNum = i;
            args[i].
                    numSources = numSources;
            args[i].
                    lockType = lockType;
            args[i].
                    processed = 0;
            if (lockType == 1) {
                clhLockTail = (qnode *) malloc(sizeof(qnode));
                clhLockTail->
                        locked = 0;
                clhLockTail->
                        id = 99999;
                clhLockTail->
                        pred = NULL;
            }
            if (strategy == 0) {
                if (pthread_create(&threads[i], NULL, workerLockFree, &args[i])) {
                    printf("Could not create a thread %d\n", i);
                    free(threads);
                    return -1;
                }
            } else if (strategy == 1) {
                if (pthread_create(&threads[i], NULL, homeQueue, &args[i])) {
                    printf("Could not create a thread %d\n", i);
                    free(threads);
                    return -1;
                }
            } else {
                if (pthread_create(&threads[i], NULL, awesomeQueue, &args[i])) {
                    printf("Could not create a thread %d\n", i);
                    free(threads);
                    return -1;
                }
            }
        }

        dispatcher(numSources, packetSource, uniform, numMilliseconds
        );

        for (
                i = 0;
                i < numSources;
                i++)
            pthread_join(threads[i], NULL);
        free(threads);

        double totalProcessed = 0;
        for (
                i = 0;
                i < numSources;
                i++) {
            totalProcessed += args[i].
                    processed;
        }

        free(args);
        free((int *) endFlags);
        free(queues);
        if (lockType == 1) {
            free((void *) clhLockTail);
        }

        printf("Number packets processed in %d seconds is %f.\n", numMilliseconds / 1000, totalProcessed);

    }

}
