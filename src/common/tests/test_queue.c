#include "unity.h"

#include "queue.h"

#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define BLOCK_CHECK_NS 100000000L
#define MIN_CONSUMERS 4
#define SPMC_ITEMS 10000
#define SPMC_CAPACITY 2
#define WRAPAROUND_ITEMS 10
#define WRAPAROUND_CAP 3
#define CAPACITY_ONE_LOOP 10

static queue *q;

typedef struct {
    queue       *q;
    queue_item   item;
    queue_status status;
    atomic_int   done;
} single_op_args;

typedef struct {
    queue        *q;
    int           item_count;
    bool          use_try_push;
    queue_status *statuses;
} producer_args;

typedef struct {
    queue *q;
    int   *ids;
    int    popped;
} consumer_args;

void setUp(void) {
    q = NULL;
}

void tearDown(void) {
    free_queue(q);
}

static queue_item make_item(int id) {
    queue_item item = {0};
    item.fd         = id;
    item.route      = ROUTE_FAST;
    return item;
}

static void sleep_for_block_check(void) {
    struct timespec delay = {0, BLOCK_CHECK_NS};
    while (nanosleep(&delay, &delay) == -1 && errno == EINTR) {
        continue;
    }
}

static int consumer_count(void) {
    long cores = sysconf(_SC_NPROCESSORS_ONLN);
    int  count = (int)cores * 2;
    return count < MIN_CONSUMERS ? MIN_CONSUMERS : count;
}

static void *push_one(void *arg) {
    single_op_args *args = arg;
    args->status         = queue_push(args->q, args->item);
    atomic_store(&args->done, 1);
    return NULL;
}

static void *pop_one(void *arg) {
    single_op_args *args = arg;
    args->status         = queue_pop(args->q, &args->item);
    atomic_store(&args->done, 1);
    return NULL;
}

static void *produce(void *arg) {
    producer_args *args = arg;
    for (int id = 0; id < args->item_count; id++) {
        queue_item item = make_item(id);
        args->statuses[id] =
            args->use_try_push ? queue_try_push(args->q, item) : queue_push(args->q, item);
    }
    return NULL;
}

static void *consume(void *arg) {
    consumer_args *args = arg;
    queue_item     item;
    while (queue_pop(args->q, &item) == QUEUE_OK) {
        args->ids[args->popped] = item.fd;
        args->popped++;
    }
    return NULL;
}

static void assert_pop_equals(int expected_id) {
    queue_item item;
    TEST_ASSERT_EQUAL_INT(QUEUE_OK, queue_pop(q, &item));
    TEST_ASSERT_EQUAL_INT(expected_id, item.fd);
}

static void fill_queue(int capacity) {
    for (int id = 0; id < capacity; id++) {
        TEST_ASSERT_EQUAL_INT(QUEUE_OK, queue_push(q, make_item(id)));
    }
}

static void start_thread(pthread_t *thread, void *(*fn)(void *), void *arg) {
    TEST_ASSERT_EQUAL_INT(0, pthread_create(thread, NULL, fn, arg));
}

// Runs 1 producer against consumer_count() consumers, then checks every id
// the producer got QUEUE_OK for was popped exactly once and in FIFO order.
static void run_spmc(bool use_try_push) {
    q = initialize_queue(QUEUE_FIFO, SPMC_CAPACITY);
    TEST_ASSERT_NOT_NULL(q);

    int            consumers = consumer_count();
    queue_status  *statuses  = calloc(SPMC_ITEMS, sizeof(queue_status));
    int           *seen      = calloc(SPMC_ITEMS, sizeof(int));
    pthread_t     *threads   = calloc((size_t)consumers, sizeof(pthread_t));
    consumer_args *cargs     = calloc((size_t)consumers, sizeof(consumer_args));
    TEST_ASSERT_NOT_NULL(statuses);
    TEST_ASSERT_NOT_NULL(seen);
    TEST_ASSERT_NOT_NULL(threads);
    TEST_ASSERT_NOT_NULL(cargs);

    for (int i = 0; i < consumers; i++) {
        cargs[i].q   = q;
        cargs[i].ids = calloc(SPMC_ITEMS, sizeof(int));
        TEST_ASSERT_NOT_NULL(cargs[i].ids);
        start_thread(&threads[i], consume, &cargs[i]);
    }

    pthread_t     producer;
    producer_args pargs = {q, SPMC_ITEMS, use_try_push, statuses};
    start_thread(&producer, produce, &pargs);

    pthread_join(producer, NULL);
    shutdown_queue(q);
    for (int i = 0; i < consumers; i++) {
        pthread_join(threads[i], NULL);
    }

    for (int i = 0; i < consumers; i++) {
        for (int j = 0; j < cargs[i].popped; j++) {
            seen[cargs[i].ids[j]]++;
            if (j > 0) {
                TEST_ASSERT_GREATER_THAN_INT(cargs[i].ids[j - 1], cargs[i].ids[j]);
            }
        }
    }

    for (int id = 0; id < SPMC_ITEMS; id++) {
        if (use_try_push) {
            TEST_ASSERT_TRUE(statuses[id] == QUEUE_OK || statuses[id] == QUEUE_FULL);
        } else {
            TEST_ASSERT_EQUAL_INT(QUEUE_OK, statuses[id]);
        }
        TEST_ASSERT_EQUAL_INT(statuses[id] == QUEUE_OK ? 1 : 0, seen[id]);
    }

    for (int i = 0; i < consumers; i++) {
        free(cargs[i].ids);
    }
    free(cargs);
    free(threads);
    free(seen);
    free(statuses);
}

void test_queue_pop_order(void) {
    q = initialize_queue(QUEUE_FIFO, 3);
    TEST_ASSERT_NOT_NULL(q);

    fill_queue(3);

    assert_pop_equals(0);
    assert_pop_equals(1);
    assert_pop_equals(2);
}

void test_queue_push_wraparound(void) {
    q = initialize_queue(QUEUE_FIFO, WRAPAROUND_CAP);
    TEST_ASSERT_NOT_NULL(q);

    int next_push = 0;
    int next_pop  = 0;

    while (next_push < WRAPAROUND_CAP) {
        TEST_ASSERT_EQUAL_INT(QUEUE_OK, queue_push(q, make_item(next_push++)));
    }

    while (next_push < WRAPAROUND_ITEMS) {
        assert_pop_equals(next_pop++);
        TEST_ASSERT_EQUAL_INT(QUEUE_OK, queue_push(q, make_item(next_push++)));
    }

    while (next_pop < WRAPAROUND_ITEMS) {
        assert_pop_equals(next_pop++);
    }
}

void test_queue_push_blocks_when_full(void) {
    q = initialize_queue(QUEUE_FIFO, 2);
    TEST_ASSERT_NOT_NULL(q);
    fill_queue(2);

    pthread_t      pusher;
    single_op_args args = {.q = q, .item = make_item(99)};
    start_thread(&pusher, push_one, &args);

    sleep_for_block_check();
    bool blocked_while_full = atomic_load(&args.done) == 0;

    queue_item   item;
    queue_status first_pop = queue_pop(q, &item);
    pthread_join(pusher, NULL);

    TEST_ASSERT_TRUE(blocked_while_full);
    TEST_ASSERT_EQUAL_INT(QUEUE_OK, first_pop);
    TEST_ASSERT_EQUAL_INT(0, item.fd);
    TEST_ASSERT_EQUAL_INT(QUEUE_OK, args.status);
    assert_pop_equals(1);
    assert_pop_equals(99);
}

void test_queue_pop_blocks_when_empty(void) {
    q = initialize_queue(QUEUE_FIFO, 2);
    TEST_ASSERT_NOT_NULL(q);

    pthread_t      popper;
    single_op_args args = {.q = q};
    start_thread(&popper, pop_one, &args);

    sleep_for_block_check();
    bool blocked_while_empty = atomic_load(&args.done) == 0;

    queue_status push_status = queue_push(q, make_item(42));
    pthread_join(popper, NULL);

    TEST_ASSERT_TRUE(blocked_while_empty);
    TEST_ASSERT_EQUAL_INT(QUEUE_OK, push_status);
    TEST_ASSERT_EQUAL_INT(QUEUE_OK, args.status);
    TEST_ASSERT_EQUAL_INT(42, args.item.fd);
}

void test_queue_spmc_exactly_once(void) {
    run_spmc(false);
}

void test_shutdown_queue_wakes_poppers(void) {
    q = initialize_queue(QUEUE_FIFO, 2);
    TEST_ASSERT_NOT_NULL(q);

    int             poppers = consumer_count();
    pthread_t      *threads = calloc((size_t)poppers, sizeof(pthread_t));
    single_op_args *args    = calloc((size_t)poppers, sizeof(single_op_args));
    TEST_ASSERT_NOT_NULL(threads);
    TEST_ASSERT_NOT_NULL(args);

    for (int i = 0; i < poppers; i++) {
        args[i].q = q;
        start_thread(&threads[i], pop_one, &args[i]);
    }

    sleep_for_block_check();
    shutdown_queue(q);
    for (int i = 0; i < poppers; i++) {
        pthread_join(threads[i], NULL);
    }

    for (int i = 0; i < poppers; i++) {
        TEST_ASSERT_EQUAL_INT(QUEUE_SHUTDOWN, args[i].status);
    }

    free(args);
    free(threads);
}

void test_queue_pop_drain(void) {
    q = initialize_queue(QUEUE_FIFO, 3);
    TEST_ASSERT_NOT_NULL(q);
    fill_queue(3);

    shutdown_queue(q);

    assert_pop_equals(0);
    assert_pop_equals(1);
    assert_pop_equals(2);

    queue_item item;
    TEST_ASSERT_EQUAL_INT(QUEUE_SHUTDOWN, queue_pop(q, &item));
}

void test_shutdown_queue_wakes_pusher(void) {
    q = initialize_queue(QUEUE_FIFO, 2);
    TEST_ASSERT_NOT_NULL(q);
    fill_queue(2);

    pthread_t      pusher;
    single_op_args args = {.q = q, .item = make_item(99)};
    start_thread(&pusher, push_one, &args);

    sleep_for_block_check();
    shutdown_queue(q);
    pthread_join(pusher, NULL);

    TEST_ASSERT_EQUAL_INT(QUEUE_SHUTDOWN, args.status);
}

void test_queue_pop_after_drained(void) {
    q = initialize_queue(QUEUE_FIFO, 2);
    TEST_ASSERT_NOT_NULL(q);

    shutdown_queue(q);

    queue_item item;
    for (int i = 0; i < 3; i++) {
        TEST_ASSERT_EQUAL_INT(QUEUE_SHUTDOWN, queue_pop(q, &item));
    }
}

void test_shutdown_queue_rejected_pusher_keeps_items(void) {
    q = initialize_queue(QUEUE_FIFO, 2);
    TEST_ASSERT_NOT_NULL(q);
    fill_queue(2);

    pthread_t      pusher;
    single_op_args args = {.q = q, .item = make_item(99)};
    start_thread(&pusher, push_one, &args);

    sleep_for_block_check();
    shutdown_queue(q);
    pthread_join(pusher, NULL);

    TEST_ASSERT_EQUAL_INT(QUEUE_SHUTDOWN, args.status);
    assert_pop_equals(0);
    assert_pop_equals(1);

    queue_item item;
    TEST_ASSERT_EQUAL_INT(QUEUE_SHUTDOWN, queue_pop(q, &item));
}

void test_queue_try_push_full(void) {
    q = initialize_queue(QUEUE_FIFO, 2);
    TEST_ASSERT_NOT_NULL(q);
    fill_queue(2);

    TEST_ASSERT_EQUAL_INT(QUEUE_FULL, queue_try_push(q, make_item(99)));

    shutdown_queue(q);
    assert_pop_equals(0);
    assert_pop_equals(1);

    queue_item item;
    TEST_ASSERT_EQUAL_INT(QUEUE_SHUTDOWN, queue_pop(q, &item));
}

void test_queue_push_after_shutdown(void) {
    q = initialize_queue(QUEUE_FIFO, 3);
    TEST_ASSERT_NOT_NULL(q);

    shutdown_queue(q);

    TEST_ASSERT_EQUAL_INT(QUEUE_SHUTDOWN, queue_push(q, make_item(1)));
    TEST_ASSERT_EQUAL_INT(QUEUE_SHUTDOWN, queue_try_push(q, make_item(2)));

    queue_item item;
    TEST_ASSERT_EQUAL_INT(QUEUE_SHUTDOWN, queue_pop(q, &item));
}

void test_queue_capacity_one(void) {
    q = initialize_queue(QUEUE_FIFO, 1);
    TEST_ASSERT_NOT_NULL(q);

    for (int id = 0; id < CAPACITY_ONE_LOOP; id++) {
        TEST_ASSERT_EQUAL_INT(QUEUE_OK, queue_push(q, make_item(id)));
        TEST_ASSERT_EQUAL_INT(QUEUE_FULL, queue_try_push(q, make_item(-1)));
        assert_pop_equals(id);
    }
}

void test_queue_spmc_try_push(void) {
    run_spmc(true);
}

void test_initialize_queue_rejects_invalid(void) {
    errno = 0;
    TEST_ASSERT_NULL(initialize_queue(QUEUE_FIFO, 0));
    TEST_ASSERT_EQUAL_INT(EINVAL, errno);

    errno = 0;
    TEST_ASSERT_NULL(initialize_queue(QUEUE_FIFO, (size_t)SEM_VALUE_MAX + 1));
    TEST_ASSERT_EQUAL_INT(EINVAL, errno);

    errno = 0;
    TEST_ASSERT_NULL(initialize_queue((queue_policy)99, 3));
    TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_queue_pop_order);
    RUN_TEST(test_queue_push_wraparound);
    RUN_TEST(test_queue_push_blocks_when_full);
    RUN_TEST(test_queue_pop_blocks_when_empty);
    RUN_TEST(test_queue_spmc_exactly_once);
    RUN_TEST(test_shutdown_queue_wakes_poppers);
    RUN_TEST(test_queue_pop_drain);
    RUN_TEST(test_shutdown_queue_wakes_pusher);
    RUN_TEST(test_queue_pop_after_drained);
    RUN_TEST(test_shutdown_queue_rejected_pusher_keeps_items);
    RUN_TEST(test_queue_try_push_full);
    RUN_TEST(test_queue_push_after_shutdown);
    RUN_TEST(test_queue_capacity_one);
    RUN_TEST(test_queue_spmc_try_push);
    RUN_TEST(test_initialize_queue_rejects_invalid);
    return UNITY_END();
}
