#include "queue_priority.h"

#include <stdbool.h>

// many of these functions were found online from a forum talking about
// implementing a priority queue using a binary heap.

static bool is_higher_priority(const queue_item a, const queue_item b) {
    if (a.priority == b.priority) {
        // If priorities are equal, use sequence number to maintain FIFO order
        return a.sequence < b.sequence;
    }

    return a.priority < b.priority;
}

static void swap_items(const queue_item *a, const queue_item *b) {
    queue_item temp = *a;
    *a              = *b;
    *b              = temp;
}

static size_t left_child_of(size_t index) {
    return 2 * index + 1;
}

static size_t right_child_of(size_t index) {
    return 2 * index + 2;
}

static size_t parent_of(size_t index) {
    return (index - 1) / 2;
}

// following 2 are heapify functions
// I kind of understand, but its about reordering on
// insert/removal of items to maintain priority
static void heapify_up(queue_item *items, size_t index) {
    while (index > 0 && is_higher_priority(items[index], items[parent_of(index)])) {
        swap_items(&items[index], &items[parent_of(index)]);
        index = parent_of(index);
    }
}

static void heapify_down(queue_item *items, size_t index, size_t count) {
    while (left_child_of(index) < count) {
        // assume left child is the highest priority
        size_t child       = left_child_of(index);
        size_t right_child = right_child_of(index);

        // check if right child exists and is higher priority than left child
        if (right_child < count && is_higher_priority(items[right_child], items[child])) {
            child = right_child;
        }

        // if the current item is higher priority than the highest priority child, we're done
        if (is_higher_priority(items[index], items[child])) {
            break;
        }

        // otherwise swap priorities and continue down the heap
        swap_items(&items[index], &items[child]);
        index = child;
    }
}

void insert_item_priority(queue *q, queue_item item) {
    item.sequence      = q->next_sequence++;
    q->items[q->count] = item;
    heapify_up(q->items, q->count);
    q->count++;
}

void remove_item_priority(queue *q, queue_item *out) {
    *out = q->items[0];
    q->count--;
    q->items[0] = q->items[q->count];
    heapify_down(q->items, 0, q->count);
}