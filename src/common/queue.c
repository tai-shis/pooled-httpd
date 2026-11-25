#include "queue.h"
#include "constants.h"

#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#include <limits.h>

static int insert_item(queue *q, queue_item item) {
}

static int remove_item(queue *q, queue_item *out) {
}

static int valid_policy(queue_policy policy) {
    return policy == QUEUE_FIFO || policy == QUEUE_PRIORITY;
}

static int initialize_queue_semaphores(queue *q) {
    if (sem_init(&q->mutex, 0, 1) == ERROR) {
        return -1;
    }

    if (sem_init(&q->empty_slots, 0, (unsigned int)q->capacity) == ERROR) {
        sem_destroy(&q->mutex);
        return -1;
    }

    if (sem_init(&q->full_slots, 0, 0) == ERROR) {
        sem_destroy(&q->empty_slots);
        sem_destroy(&q->mutex);
        return -1;
    }

    return 0;
}

queue *initialize_queue(queue_policy policy, size_t capacity) {
    if (!valid_policy(policy)) {
        errno = EINVAL;
        return NULL;
    }

    if (capacity == 0 || capacity > SEM_VALUE_MAX) {
        errno = EINVAL;
        return NULL;
    }

    queue *q = malloc(sizeof(queue));

    if (q == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    q->items = calloc(capacity, sizeof(queue_item));

    if (q->items == NULL) {
        free(q);
        errno = ENOMEM;
        return NULL;
    }

    q->policy   = policy;
    q->capacity = capacity;
    q->head     = 0;
    q->tail     = 0;
    q->count    = 0;
    q->shutdown = false;

    if (initialize_queue_semaphores(q) == ERROR) {
        free(q->items);
        free(q);
        return NULL;
    }

    return q;
}

void free_queue(queue *q);

queue_status queue_push(queue *q, queue_item item);
queue_status queue_try_push(queue *q, queue_item item);
queue_status queue_pop(queue *q, queue_item *out);
void         shutdown_queue(queue *q);
