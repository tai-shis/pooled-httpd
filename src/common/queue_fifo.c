#include "queue_fifo.h"

void insert_item_fifo(queue *q, queue_item item) {
    // assuming critical section preserved
    q->items[q->tail] = item;
    q->tail           = (q->tail + 1) % q->capacity;
    q->count++;
}
