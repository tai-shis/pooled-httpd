#include "queue.h"
#include "constants.h"
#include "queue_fifo.h"
#include "commonlib.h"

#include <stdlib.h>
#include <errno.h>
#include <limits.h>

static int valid_policy(queue_policy policy) {
    return policy == QUEUE_FIFO || policy == QUEUE_PRIORITY;
}

static int initialize_queue_semaphores(queue *q) {
    if (sem_init(&q->mutex, 0, 1) == ERROR) {
        return ERROR;
    }

    if (sem_init(&q->empty_slots, 0, (unsigned int)q->capacity) == ERROR) {
        sem_destroy(&q->mutex);
        return ERROR;
    }

    if (sem_init(&q->full_slots, 0, 0) == ERROR) {
        sem_destroy(&q->empty_slots);
        sem_destroy(&q->mutex);
        return ERROR;
    }

    return SUCCESS;
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
        return NULL;
    }

    q->items = calloc(capacity, sizeof(queue_item));

    if (q->items == NULL) {
        free(q);
        return NULL;
    }

    q->policy        = policy;
    q->capacity      = capacity;
    q->head          = 0;
    q->tail          = 0;
    q->count         = 0;
    q->shutdown      = false;
    q->next_sequence = 0;

    if (initialize_queue_semaphores(q) == ERROR) {
        free(q->items);
        free(q);
        return NULL;
    }

    return q;
}

void free_queue(queue *q) {
    if (q == NULL) {
        return;
    }

    sem_destroy(&q->full_slots);
    sem_destroy(&q->empty_slots);
    sem_destroy(&q->mutex);
    free(q->items);
    free(q);
}

static void insert_item(queue *q, queue_item item) {
    switch (q->policy) {
    case QUEUE_FIFO:
        insert_item_fifo(q, item);
        break;
    case QUEUE_PRIORITY:
        break;
    default:
        break;
    }
}

static int64_t timespec_to_ns(const struct timespec *ts) {
    return (int64_t)ts->tv_sec * NS_PER_S + (int64_t)ts->tv_nsec;
}

static void populate_priority_fields(queue_item *item) {
    clock_gettime(CLOCK_MONOTONIC, &item->enqueue_time);
    item->priority = timespec_to_ns(&item->enqueue_time) + item->cost_ns;
}

queue_status queue_push(queue *q, queue_item item) {
    if (q == NULL) {
        return QUEUE_SHUTDOWN;
    }

    wait_semaphore(&q->empty_slots);

    populate_priority_fields(&item);

    wait_semaphore(&q->mutex);
    if (q->shutdown) {
        post_semaphore(&q->mutex);
        post_semaphore(&q->empty_slots);
        return QUEUE_SHUTDOWN;
    }

    insert_item(q, item);

    post_semaphore(&q->mutex);
    post_semaphore(&q->full_slots);

    return QUEUE_OK;
}

queue_status queue_try_push(queue *q, queue_item item) {
    if (q == NULL) {
        return QUEUE_SHUTDOWN;
    }

    while (sem_trywait(&q->empty_slots) == ERROR) {
        if (errno == EAGAIN) {
            return QUEUE_FULL;
        } else if (errno != EINTR) {
            return QUEUE_SHUTDOWN;
        }
    }

    populate_priority_fields(&item);

    wait_semaphore(&q->mutex);
    if (q->shutdown) {
        post_semaphore(&q->mutex);
        post_semaphore(&q->empty_slots);
        return QUEUE_SHUTDOWN;
    }

    insert_item(q, item);

    post_semaphore(&q->mutex);
    post_semaphore(&q->full_slots);

    return QUEUE_OK;
}

static void remove_item(queue *q, queue_item *out) {
    switch (q->policy) {
    case QUEUE_FIFO:
        remove_item_fifo(q, out);
        break;
    case QUEUE_PRIORITY:
        break;
    default:
        break;
    }
}

queue_status queue_pop(queue *q, queue_item *out) {
    if (q == NULL) {
        return QUEUE_SHUTDOWN;
    }

    wait_semaphore(&q->full_slots);
    wait_semaphore(&q->mutex);

    if (q->count > 0) {
        remove_item(q, out);
        post_semaphore(&q->mutex);
        post_semaphore(&q->empty_slots);
        return QUEUE_OK;
    }

    post_semaphore(&q->mutex);
    post_semaphore(&q->full_slots); // to wake back up other poppers
    return QUEUE_SHUTDOWN;
}

void shutdown_queue(queue *q) {
    if (q == NULL) {
        return;
    }

    wait_semaphore(&q->mutex);
    q->shutdown = true;
    post_semaphore(&q->mutex);

    post_semaphore(&q->empty_slots);
    post_semaphore(&q->full_slots);
}
