#ifndef QUEUE_H
#define QUEUE_H

#include "queue_item.h"

#include <semaphore.h>

typedef struct queue queue;

typedef enum {
    QUEUE_FIFO,
    QUEUE_PRIORITY
} queue_policy;

typedef enum {
    QUEUE_OK       = 0,
    QUEUE_FULL     = -1, /* try_push: at capacity, caller decides (e.g. 503) */
    QUEUE_SHUTDOWN = -2  /* (try)push is rejected / pop: shut down and drained */
} queue_status;

struct queue {
    queue_policy policy;   // readonly after init
    queue_item  *items;    // protected by mutex
    size_t       capacity; // readonly after init
    size_t       head;     // FIFO ONLY, protected by mutex
    size_t       tail;     // FIFO ONLY, protected by mutex
    size_t       count;    // protected by mutex
    int          shutdown; // protected by mutex
    sem_t        mutex;
    sem_t        empty_slots;
    sem_t        full_slots;
};

/**
 * @brief Initializes a queue with the given policy and capacity
 * @param policy The policy to use for the queue
 * @param capacity The maximum number of items the queue can hold
 * @return A pointer to the initialized queue, or NULL if initialization fails
 */
queue *initialize_queue(queue_policy policy, size_t capacity);

/**
 * @brief Frees the memory allocated as well as the semaphores used by the queue
 * Call only after shutdown_queue() has been called and all threads are awake/alive
 * to avoid destroying a semaphore that a thread is waiting on.
 *
 * @param q The queue to free
 */
void free_queue(queue *q);

queue_status queue_push(queue *q, queue_item item);     /* blocks */
queue_status queue_try_push(queue *q, queue_item item); /* non-blocking */
queue_status queue_pop(queue *q, queue_item *out);      /* blocks until item or shutdown */
void         shutdown_queue(queue *q);                  /* wakes all blocked pushers/poppers */

#endif