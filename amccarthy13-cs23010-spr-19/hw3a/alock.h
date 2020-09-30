//
// Created by amccarthy13 on 5/10/19.
//

#ifndef CS_23010_ALOCK_H
#define CS_23010_ALOCK_H

#include <stdbool.h>

extern volatile int aLockTail;
extern volatile int *aLockFlag;

void alock_lock(int size, int *currSlot);
void alock_unlock(int size, int currSlot);
bool alock_tryLock(int size, int *currSlot);


#endif //CS_23010_ALOCK_H
