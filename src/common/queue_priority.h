#ifndef QUEUE_PRIORITY_H
#define QUEUE_PRIORITY_H

// purposely not in include to be private

#include "queue.h"

/**
 * @brief Inserts an item into the queue assuming Priority policy
 * Caller must hold the queue's mutex before calling this function.
 * Assumes that priority is set.
 *
 * @param q The queue to insert into
 * @param item The item to insert
 */
void insert_item_priority(queue *q, queue_item item);

/**
 * @brief Removes an item from the queue assuming Priority policy
 * Caller must hold the queue's mutex before calling this function.
 * Assumes that priority is set and count > 0.
 *
 *
 * @param q The queue to remove from
 * @param out The location to store the removed item
 */
void remove_item_priority(queue *q, queue_item *out);

#endif