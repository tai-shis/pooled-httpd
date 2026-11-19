#ifndef QUEUE_H
#define QUEUE_H

#include "queue_item.h"

typedef struct queue queue;   /* opaque */

typedef enum {
    QUEUE_FIFO,
    QUEUE_PRIORITY
} queue_policy;

typedef enum {
    QUEUE_OK       =  0,
    QUEUE_FULL     = -1,   /* push: at capacity, caller decides (e.g. 503) */
    QUEUE_SHUTDOWN = -2    /* push: rejected / pop: shut down and drained */
} queue_status;

queue       *queue_create(queue_policy policy, size_t capacity);
void         queue_destroy(queue *q);

queue_status queue_push(queue *q, queue_item item);   /* non-blocking */
queue_status queue_pop(queue *q, queue_item *out);    /* blocks until item or shutdown */
void         queue_shutdown(queue *q);                /* wakes all blocked poppers */

#endif