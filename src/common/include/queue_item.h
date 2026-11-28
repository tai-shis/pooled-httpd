#ifndef QUEUE_ITEM_H
#define QUEUE_ITEM_H

#include "routes.h"

#include <time.h>
#include <stdint.h>

typedef struct {
    int             fd; // accpted client socket
    route_id        route;
    struct timespec enqueue_time; // CLOCK_MONOTONIC, set by push
    int64_t         priority;     // priority policy only (enqueue_time + cost); FIFO ignores it
    int64_t         cost_ns;      // cost in nanoseconds (w * tao), set by acceptor
    uint64_t        sequence;     // sequence number, set by push (mutex protected)
} queue_item;

#endif