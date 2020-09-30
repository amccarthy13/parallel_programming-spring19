//
// Created by amccarthy13 on 5/11/19.
//

#include <stdbool.h>
#include "clhlock.h"

void clh_lock(qnode **currnode) {
    (*currnode)->locked = 1;
    volatile qnode *pred = __sync_lock_test_and_set(&clhLockTail, *currnode);
    (*currnode)->pred = (qnode *) pred;
    while (pred->locked) {;}

}

void clh_unlock(qnode **currnode) {
    qnode *pred = (*currnode)->pred;
    (*currnode)->locked = 0;
    *currnode = pred;
}

bool clh_tryLock(qnode **currnode) {
    if (!clhLockTail->locked) {
        (*currnode)->locked = 1;
        volatile qnode *pred = __sync_lock_test_and_set(&clhLockTail, *currnode);
        (*currnode)->pred = (qnode *) pred;
        return true;
    } else
        return false;
}