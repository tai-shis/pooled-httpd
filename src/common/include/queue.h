#ifndef QUEUE_H
#define QUEUE_H

#include "queue_item.h"

#include <semaphore.h>
#include <stdbool.h>

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
    bool         shutdown; // protected by mutex
    sem_t        mutex;
    sem_t        empty_slots;
    sem_t        full_slots;
};

/**
 * @brief Initializes a queue with the given policy and capacity
 * @param policy The policy to use for the queue (QUEUE_FIFO or QUEUE_PRIORITY)
 * @param capacity The maximum number of items the queue can hold (must be > 0 and <= SEM_VALUE_MAX)
 * @return A pointer to the initialized queue, or NULL if initialization fails, setting errno to
 * EINVAL
 */
queue *initialize_queue(queue_policy policy, size_t capacity);

/**
 * @brief Frees the memory allocated as well as the semaphores used by the queue
 * Call only after shutdown_queue() has been called and all threads have joined
 * to avoid destroying a semaphore that a thread is waiting on.
 *
 * @param q The queue to free
 */
void free_queue(queue *q);

/**
 * @brief Pushes an item onto the queue, blocking if the queue is full
 * @param q The queue to push onto
 * @param item The item to push onto the queue
 *
 * @return QUEUE_OK on success, QUEUE_SHUTDOWN if the queue is shut down
 */
queue_status queue_push(queue *q, queue_item item); /* blocks */

/**
 * @brief Tries to push an item onto the queue, returning immediately if the queue is full
 * @param q The queue to push onto
 * @param item The item to push onto the queue
 *
 * @return QUEUE_OK on success, QUEUE_FULL if the queue is full, QUEUE_SHUTDOWN if the queue is shut
 * down
 */
queue_status queue_try_push(queue *q, queue_item item); /* non-blocking */

queue_status queue_pop(queue *q, queue_item *out); /* blocks until item or shutdown */
void         shutdown_queue(queue *q);             /* wakes all blocked pushers/poppers */

#endif