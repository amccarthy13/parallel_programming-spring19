//
// Created by amccarthy13 on 5/11/19.
//

#ifndef CS_23010_CLHLOCK_H
#define CS_23010_CLHLOCK_H

#include <stdbool.h>

typedef struct qnode {
    int locked;
    int id;
    struct qnode *pred;
} qnode;

extern volatile qnode *clhLockTail;

void clh_lock(qnode **currnode);

void clh_unlock(qnode **currnode);

bool clh_tryLock(qnode **currnode);

#endif //CS_23010_CLHLOCK_H
