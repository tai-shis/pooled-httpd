#ifndef COMMONLIB_H
#define COMMONLIB_H

#include <semaphore.h>

/**
 * @brief Waits on a semaphore, retrying if interrupted by a signal
 *
 * @param sem The semaphore to wait on
 */
void wait_semaphore(sem_t *sem);

/**
 * @brief Posts to a semaphore
 *
 * @param sem The semaphore to post to
 */
void post_semaphore(sem_t *sem);

#endif
