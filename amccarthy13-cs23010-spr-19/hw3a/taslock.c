//
// Created by amccarthy13 on 5/14/19.
//

#include <stdbool.h>
#include "taslock.h"

volatile int taslock_lock_t;

void taslock_lock() {
    while (__sync_fetch_and_or(&taslock_lock_t, 1)){}
}

void taslock_unlock() {
    __sync_and_and_fetch(&taslock_lock_t, 0);
}

bool taslock_tryLock() {
    if (!taslock_lock_t) {
        __sync_fetch_and_or(&taslock_lock_t, 1);
        return true;
    } else
        return false;
}