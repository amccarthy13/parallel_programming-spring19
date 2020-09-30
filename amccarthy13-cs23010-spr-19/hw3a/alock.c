//
// Created by amccarthy13 on 5/10/19.
//

#include <stdbool.h>
#include "alock.h"

void alock_lock(int size, int *currSlot) {
    int slot = ((__sync_fetch_and_add(&aLockTail, 8))) % size;
    *currSlot = slot;
    while (!aLockFlag[slot]) {;}
}

void alock_unlock(int size, int currSlot) {
    int slot = currSlot;
    aLockFlag[slot] = 0;
    aLockFlag[((slot+8) % size)] = 1;
}

bool alock_tryLock(int size, int *currSlot) {
    if (aLockFlag[aLockTail]) {
        int slot = ((__sync_fetch_and_add(&aLockTail, 8))) % size;
        *currSlot = slot;
        return true;
    } else
        return false;
}

