#ifndef COMMONLIB_H
#define COMMONLIB_H

#include <semaphore.h>
#include <time.h>

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

/**
 * @brief Calculates the elapsed time in milliseconds between two timespecs
 *
 * @param start The starting timespec
 * @param end The ending timespec
 * @return The elapsed time in milliseconds, negative if end is before start
 */
double get_elapsed_ms(const struct timespec *start, const struct timespec *end);

#endif
