#include "commonlib.h"

#include <errno.h>
#include "constants.h"

void wait_semaphore(sem_t *sem) {
    while (sem_wait(sem) == ERROR && errno == EINTR) {
        continue; // retry if interrupted by signal
    }
}

void post_semaphore(sem_t *sem) {
    sem_post(sem);
}

double get_elapsed_ms(const struct timespec *start, const struct timespec *end) {
    long elapsed_ns = (end->tv_sec - start->tv_sec) * NS_PER_S + (end->tv_nsec - start->tv_nsec);
    return elapsed_ns / NS_PER_MS;
}