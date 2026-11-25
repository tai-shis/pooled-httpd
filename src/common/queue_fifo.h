#ifndef QUEUE_FIFO_H
#define QUEUE_FIFO_H

#include "queue.h"

// purposely in common to be "private"

/**
 * @brief Inserts an item into the queue assuming FIFO policy
 * Caller must hold the queue's mutex before calling this function.
 *
 * @param q The queue to insert into
 * @param item The item to insert
 */
void insert_item_fifo(queue *q, queue_item item);

/**
 * @brief Removes an item from the queue assuming FIFO policy
 * Caller must hold the queue's mutex before calling this function.
 *
 * @param q The queue to remove from
 * @param out The location to store the removed item
 */
void remove_item_fifo(queue *q, queue_item *out);

#endif