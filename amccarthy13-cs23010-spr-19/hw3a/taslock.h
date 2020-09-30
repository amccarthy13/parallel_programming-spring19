//
// Created by amccarthy13 on 5/10/19.
//

#ifndef CS_23010_TASLOCK_H
#define CS_23010_TASLOCK_H

#include <stdbool.h>

void taslock_lock();
void taslock_unlock();
bool taslock_tryLock();

#endif //CS_23010_TASLOCK_H
