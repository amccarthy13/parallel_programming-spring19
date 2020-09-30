//
// Created by amccarthy13 on 4/24/19.
//

#include <stdlib.h>

#include "lamport.h"
#include "atomic.h"

#define E(val) ((element_t *)&val)

typedef struct {
    unsigned long ptr;
    unsigned long ref;
} element_t;

struct _queue_t {
    size_t depth;
    element_t *e;
    unsigned long rear;
    unsigned long front;
};

queue_t create_queue(size_t depth)
{
    queue_t q = (queue_t)malloc(sizeof(struct _queue_t));
    if (q) {
        q->depth = depth;
        q->rear = q->front = 0;
        q->e = (element_t *)calloc(depth, sizeof(element_t));
    }
    return q;
}

int enqueue_queue(volatile queue_t q, volatile void *data) {
    DWORD old, new;
    unsigned long tail, head;
    do {
        tail = q-> rear;
        old = *((DWORD *)&(q->e[tail % q->depth]));
        head = q->front;
        if (tail != q->rear)
            continue;
        if (tail == (q->front + q->depth)) {
            if (q->e[head % q->depth].ptr && head == q->front)
                return 0;
            CAS(&(q->front), head, head+1);
            continue;
        }
        if (!E(old)->ptr) {
            E(new)->ptr = (uintptr_t)data;
            E(new)->ref = ((element_t *)&old)->ref + 1;
            if (DWCAS((DWORD *)&(q->e[tail % q->depth]), old, new)) {
                CAS(&(q->rear), tail, tail+1);
                return 1;
            }
        } else if (q->e[tail % q->depth].ptr)
            CAS(&(q->rear), tail, tail+1);
    } while(1);
}

volatile void *dequeue_queue(volatile queue_t q) {
    DWORD old, new;
    unsigned long head, tail;
    do {
        head = q->front;
        old = *((DWORD *)&(q->e[head % q->depth]));
        tail = q->rear;
        if (head != q->front)
            continue;
        if (head == q->rear) {
            if (!q->e[tail % q->depth].ptr && tail == q->rear)
                return NULL;
            CAS(&(q->rear), tail, tail + 1);
            continue;
        }
        if (E(old)->ptr) {
            E(new)->ptr = 0;
            E(new)->ref = E(old)->ref + 1;
            if (DWCAS((DWORD *)&(q->e[head % q->depth]), old, new)) {
                CAS(&(q->front), head, head+1);
                return (void *)((element_t *)&old)->ptr;
            }
        } else if (!q->e[head % q->depth].ptr)
            CAS(&(q->front), head, head + 1);
    } while(1);
}