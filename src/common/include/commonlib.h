#ifndef COMMONLIB_H
#define COMMONLIB_H

#include <semaphore.h>

void wait_semaphore(sem_t *sem);

void post_semaphore(sem_t *sem);

#endif
