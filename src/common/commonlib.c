#include "commonlib.h"

#include <errno.h>
#include <constants.h>

void wait_semaphore(sem_t *sem) {
    while (sem_wait(sem) == ERROR && errno == EINTR) {
        continue; // retry if interrupted by signal
    }
}

void post_semaphore(sem_t *sem) {
    sem_post(sem);
}