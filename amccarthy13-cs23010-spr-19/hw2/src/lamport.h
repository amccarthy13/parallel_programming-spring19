//
// Created by amccarthy13 on 4/24/19.
//

#ifndef CS_23010_LAMPORT_H
#define CS_23010_LAMPORT_H

#include <stdint.h>
#include <stdlib.h>

typedef struct _queue_t *queue_t;

queue_t create_queue(size_t);
volatile void *dequeue_queue(queue_t);
int enqueue_queue(queue_t, volatile void *);

#endif
