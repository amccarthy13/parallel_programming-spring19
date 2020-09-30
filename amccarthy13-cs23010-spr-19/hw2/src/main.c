//
// Created by amccarthy13 on 4/27/19.
//

#include "lamport.h"
#include "fingerprint.h"
#include "packetsource.h"
#include "stopwatch.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <pthread.h>
#include <math.h>
#include <stdbool.h>
#include <unistd.h>

#define MAX 1000
queue_t queues[MAX];
int packetsToDo;

void *worker(void *arg) {
    volatile Packet_t *packet;
    int packetsLeft = packetsToDo;
    while (packetsLeft > 0) {
        packet = dequeue_queue(queues[(int) (__intptr_t) arg]);
        if (packet == NULL) {
            continue;
        } else {
            packetsLeft--;
        }
    }
    return 0;
}

void dispatcher(int numQueues, PacketSource_t *packetSource, int generationType, int expectedWork) {
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
                packet = getConstantPacket(expectedWork);
            else if (generationType == 1)
                packet = getUniformPacket(packetSource, i);
            else
                packet = getExponentialPacket(packetSource, i);
            if (enqueue_queue(queues[i], packet) == 0) {
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
}

void serialNoQueue(PacketSource_t *packetSource, int numSources, int generationType, int expectedWork) {
    int i, j;
    volatile Packet_t *packet;

    for (i = 0; i < numSources; i++) {
        for (j = 0; j < packetsToDo; j++) {
            if (generationType == 0)
                packet = getConstantPacket(expectedWork);
            else if (generationType == 1)
                packet = getUniformPacket(packetSource, i);
            else
                packet = getExponentialPacket(packetSource, i);
            getFingerprint(packet->iterations, packet->seed);
        }
    }

}

void serialQueue(PacketSource_t *packetSource, int generationType, int numSources, int expectedWork) {
    int i, j;
    volatile Packet_t *packet;


    for (i = 0; i < numSources; i++) {
        for (j = 0; j < packetsToDo; j++) {
            if (generationType == 0)
                packet = getConstantPacket(expectedWork);
            else if (generationType == 1)
                packet = getUniformPacket(packetSource, i);
            else
                packet = getExponentialPacket(packetSource, i);
            enqueue_queue(queues[i], packet);
            packet = dequeue_queue(queues[i]);
            getFingerprint(packet->iterations, packet->seed);
        }
    }
}

int main(int argc, char *argv[]) {
    pthread_t *threads;
    int seed = 10;

    if (argc < 2) {
        printf("No parameters given\n");
        return -1;
    }

    if (strcmp(argv[1], "test") == 0) {
        if (argc < 3) {
            printf("Need to provide a seed for testing\n");
        }
        seed = atoi(argv[2]);
        int w[6] = {25, 50, 100, 200, 400, 800};
        int n[3] = {2, 9, 14};
        int i, j, q, z, gen;
        StopWatch_t stopWatch;
        PacketSource_t *packetSource;
        double parallel;
        double serial;

        //queue overhead
        for (i = 0; i < 6; i++) {
            for (j = 0; j < 3; j++) {
                packetsToDo = pow(2, 24) / (n[j] * w[i]);
                packetSource = createPacketSource(w[i], n[j] - 1, seed);
                for (q = 0; q < (n[j] - 1); ++q) {
                    queues[q] = create_queue(32);
                }

                startTimer(&stopWatch);
                serialQueue(packetSource, 1, (n[j] - 1), w[i]);
                stopTimer(&stopWatch);
                parallel = getElapsedTime(&stopWatch);

                packetSource = createPacketSource(w[i], n[j] - 1, seed);

                startTimer(&stopWatch);
                serialNoQueue(packetSource, (n[j] - 1), 1, w[i]);
                stopTimer(&stopWatch);

                serial = getElapsedTime(&stopWatch);
                printf("speedup queue overhead with w = %d and n = %d is %lf\n", w[i], n[j], parallel/serial);
                printf("queue run time with with w = %d and n = %d is %lf\n", w[i], n[j], parallel);
            }
        }

        //dispatcher rate
        int n2[6] = {2, 3, 5, 9, 14, 28};
        for (i = 0; i < 6; i++) {
            packetsToDo = pow(2, 24) / (n2[i] - 1);

            threads = (pthread_t *) malloc((n2[i] - 1) * sizeof(pthread_t));

            for (q = 0; q < (n2[i] - 1); ++q) {
                queues[q] = create_queue(32);
            }
            packetSource = createPacketSource(1, n2[i] - 1, seed);

            startTimer(&stopWatch);
            for (z = 0; z < (n2[i] - 1); ++z) {
                if (pthread_create(&threads[z], NULL, &worker, (void *) (__intptr_t) z)) {
                    printf("Could not create a thread %d\n", z);
                    free(threads);
                    return -1;
                }
            }

            dispatcher(n2[i] - 1, packetSource, 1, 1);

            for (q = 0; q < (n2[i] - 1); q++) {
                pthread_join(threads[q], NULL);
            }
            free(threads);

            stopTimer(&stopWatch);

            printf("dispatcher rate time with w = 1 and n = %d is %lf\n", n2[i], getElapsedTime(&stopWatch));

        }


        int w3[4] = {1000, 2000, 4000, 8000};
        int n3[6] = {2, 3, 5, 9, 14, 28};
        packetsToDo = pow(2, 17);

        char *type;

        //constant, uniform, and exponential
        for (gen = 0; gen < 3; gen++) {
            if (gen == 0)
                type = "constant";
            else if (gen == 1)
                type = "uniform";
            else
                type = "exponential";
            for (i = 0; i < 4; i++) {
                for (j = 0; j < 6; j++) {
                    threads = (pthread_t *) malloc((n3[j] - 1) * sizeof(pthread_t));

                    for (q = 0; q < (n3[j] - 1); ++q) {
                        queues[q] = create_queue(32);
                    }

                    packetSource = createPacketSource(w3[i], (n3[j] - 1), seed);

                    startTimer(&stopWatch);

                    for (z = 0; z < (n3[j] - 1); ++z) {
                        if (pthread_create(&threads[z], NULL, &worker, (void *) (__intptr_t) z)) {
                            printf("Could not create a thread %d\n", z);
                            free(threads);
                            return -1;
                        }
                    }

                    dispatcher((n3[j] - 1), packetSource, gen, w3[i]);

                    for (z = 0; z < (n3[j] - 1); z++)
                        pthread_join(threads[z], NULL);
                    free(threads);

                    stopTimer(&stopWatch);

                    parallel = getElapsedTime(&stopWatch);

                    packetSource = createPacketSource(w3[i], n3[j] - 1, seed);

                    startTimer(&stopWatch);

                    serialNoQueue(packetSource, (n3[j] - 1), gen, w3[i]);

                    stopTimer(&stopWatch);

                    serial = getElapsedTime(&stopWatch);

                    printf("speedup %s packets with w = %d and n = %d is %lf\n", type, w3[i], n3[j],
                           serial/parallel);
                }
            }
        }

        return 0;
    }

    int i;
    int numThreads = 1;
    int queueDepth = 10;
    int expectedWork = 100;
    int genType = 0;
    int mode = 0;
    bool modeFlag = true;
    bool genFlag = true;
    bool threadFlag = true;
    bool packetFlag = true;
    bool depthFlag = true;
    bool workFlag = true;

    for (i = 0; i < (argc - 1); i++) {
        if (strcmp(argv[i], "-n") == 0) {
            numThreads = atoi(argv[i + 1]);
            threadFlag = false;
        }

        if (strcmp(argv[i], "-t") == 0) {
            packetsToDo = atoi(argv[i + 1]);
            packetFlag = false;
        }

        if (strcmp(argv[i], "-d") == 0) {
            queueDepth = atoi(argv[i + 1]);
            depthFlag = false;
        }

        if (strcmp(argv[i], "-w") == 0) {
            expectedWork = atoi(argv[i + 1]);
            workFlag = false;
        }

        if (strcmp(argv[i], "-s") == 0) {
            seed = atoi(argv[i + 1]);
        }

        if (strcmp(argv[i], "-g") == 0) {
            if (strcmp(argv[i + 1], "constant") == 0) {
                genType = 0;
            } else if (strcmp(argv[i + 1], "uniform") == 0) {
                genType = 1;
            } else if (strcmp(argv[i + 1], "exponential") == 0) {
                genType = 2;
            } else {
                printf("Invalid packet generation type: must be constant, uniform, or exponential\n");
                return -1;
            }
            genFlag = false;
        }
        if (strcmp(argv[i], "-m") == 0) {
            if (strcmp(argv[i + 1], "parallel") == 0) {
                mode = 0;
            } else if (strcmp(argv[i + 1], "serial") == 0) {
                mode = 1;
            } else if (strcmp(argv[i + 1], "serial-queue") == 0) {
                mode = 2;
            } else {
                printf("Invalid packet generation type: must be parallel, serial, or serial-queue\n");
                return -1;
            }
            modeFlag = false;
        }
    }

    if (threadFlag || packetFlag || depthFlag || workFlag || modeFlag || genFlag) {
        printf("Not all required parameters provided\n");
        return -1;
    }

    PacketSource_t *packetSource = createPacketSource(expectedWork, numThreads - 1, seed);

    StopWatch_t stopWatch;

    if (mode == 0) {
        startTimer(&stopWatch);
        serialNoQueue(packetSource, (numThreads - 1), genType, expectedWork);
        stopTimer(&stopWatch);
    } else if (mode == 1) {
        startTimer(&stopWatch);
        serialQueue(packetSource, genType, (numThreads - 1), expectedWork);
        stopTimer(&stopWatch);
    } else {

        for (i = 0; i < (numThreads - 1); ++i) {
            queues[i] = create_queue(queueDepth);
        }


        threads = (pthread_t *) malloc((numThreads - 1) * sizeof(pthread_t));

        startTimer(&stopWatch);

        for (i = 0; i < (numThreads - 1); ++i) {
            if (pthread_create(&threads[i], NULL, &worker, (void *) (__intptr_t) i)) {
                printf("Could not create a thread %d\n", i);
                free(threads);
                return -1;
            }
        }

        dispatcher(numThreads - 1, packetSource, genType, expectedWork);

        for (i = 0; i < (numThreads - 1); i++)
            pthread_join(threads[i], NULL);
        free(threads);

        stopTimer(&stopWatch);


    }
    printf("run time: %lf\n", getElapsedTime(&stopWatch));
}
