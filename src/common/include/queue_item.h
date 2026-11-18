#ifndef QUEUE_ITEM_H
#define QUEUE_ITEM_H

#include <time.h>
#include "routes.h"

typedef struct {
    // accpted client socket
    int             fd;            
    route_id        route;
    // CLOCK_MONOTONIC, set by push
    struct timespec enqueue_time;  
    // priority policy only; FIFO ignores it
    int             priority;  
} queue_item;

#endif