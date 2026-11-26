#include "queue_fifo.h"

void insert_item_fifo(queue *q, queue_item item) {
    // assuming critical section preserved
    q->items[q->tail] = item;
    q->tail           = (q->tail + 1) % q->capacity;
    q->count++;
}

void remove_item_fifo(queue *q, queue_item *out) {
    // assuming critical section preserved
    *out    = q->items[q->head];
    q->head = (q->head + 1) % q->capacity;
    q->count--;
}