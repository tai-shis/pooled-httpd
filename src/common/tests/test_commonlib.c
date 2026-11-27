#include "unity.h"

#include "commonlib.h"

#include <time.h>

#define ELAPSED_TOLERANCE_MS 0.0001f
#define LARGE_MONOTONIC_SEC 1000000

void setUp(void) {
}

void tearDown(void) {
}

static struct timespec make_timespec(time_t sec, long nsec) {
    struct timespec time = {sec, nsec};
    return time;
}

void test_get_elapsed_ms_equal(void) {
    struct timespec start = make_timespec(5, 250000000);
    struct timespec end   = make_timespec(5, 250000000);

    TEST_ASSERT_FLOAT_WITHIN(ELAPSED_TOLERANCE_MS, 0.0f, get_elapsed_ms(&start, &end));
}

void test_get_elapsed_ms_whole_seconds(void) {
    struct timespec start = make_timespec(1, 0);
    struct timespec end   = make_timespec(3, 0);

    TEST_ASSERT_FLOAT_WITHIN(ELAPSED_TOLERANCE_MS, 2000.0f, get_elapsed_ms(&start, &end));
}

void test_get_elapsed_ms_nanoseconds_only(void) {
    struct timespec start = make_timespec(2, 100000000);
    struct timespec end   = make_timespec(2, 100500000);

    TEST_ASSERT_FLOAT_WITHIN(ELAPSED_TOLERANCE_MS, 0.5f, get_elapsed_ms(&start, &end));
}

void test_get_elapsed_ms_borrow(void) {
    struct timespec start = make_timespec(1, 900000000);
    struct timespec end   = make_timespec(2, 100000000);

    TEST_ASSERT_FLOAT_WITHIN(ELAPSED_TOLERANCE_MS, 200.0f, get_elapsed_ms(&start, &end));
}

void test_get_elapsed_ms_large_timestamps(void) {
    struct timespec start = make_timespec(LARGE_MONOTONIC_SEC, 123456789);
    struct timespec end   = make_timespec(LARGE_MONOTONIC_SEC, 124456789);

    TEST_ASSERT_FLOAT_WITHIN(ELAPSED_TOLERANCE_MS, 1.0f, get_elapsed_ms(&start, &end));
}

void test_get_elapsed_ms_end_before_start(void) {
    struct timespec start = make_timespec(3, 0);
    struct timespec end   = make_timespec(2, 500000000);

    TEST_ASSERT_FLOAT_WITHIN(ELAPSED_TOLERANCE_MS, -500.0f, get_elapsed_ms(&start, &end));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_get_elapsed_ms_equal);
    RUN_TEST(test_get_elapsed_ms_whole_seconds);
    RUN_TEST(test_get_elapsed_ms_nanoseconds_only);
    RUN_TEST(test_get_elapsed_ms_borrow);
    RUN_TEST(test_get_elapsed_ms_large_timestamps);
    RUN_TEST(test_get_elapsed_ms_end_before_start);
    return UNITY_END();
}
