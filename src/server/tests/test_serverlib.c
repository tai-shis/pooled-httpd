#include "unity.h"

#include "constants.h"
#include "routes.h"
#include "serverlib.h"

#define SAMPLE_COUNT 10000
#define SEQUENCE_LENGTH 100
#define TEST_SEED 42u

// the ticket allows the average to be off by up to 20%
#define MEAN_TOLERANCE 0.20

// test-only route, so retuning the real table does not break these tests
static route_info make_test_route(unsigned jitter_pct, unsigned long_pct) {
    route_info route = {
        .path         = "/test",
        .kind         = ROUTE_KIND_CPU,
        .mean_ms      = 100,
        .jitter_pct   = jitter_pct,
        .long_pct     = long_pct,
        .long_mean_ms = 1000,
    };
    return route;
}

static double expected_mean_ms(const route_info *route) {
    double long_fraction = route->long_pct / PERCENT;
    return route->mean_ms * (1.0 - long_fraction) + route->long_mean_ms * long_fraction;
}

static int count_long_samples(const route_info *route, unsigned int *seed) {
    int long_count = 0;

    for (int i = 0; i < SAMPLE_COUNT; i++) {
        if (get_service_ms(route, seed) == route->long_mean_ms) {
            long_count++;
        }
    }
    return long_count;
}

void setUp(void) {
}

void tearDown(void) {
}

void test_get_service_ms_average(void) {
    for (route_id id = 0; id < ROUTE_COUNT; id++) {
        const route_info *route = route_get(id);
        unsigned int seed       = TEST_SEED;
        double total_ms         = 0.0;

        for (int i = 0; i < SAMPLE_COUNT; i++) {
            total_ms += get_service_ms(route, &seed);
        }

        double expected_ms = expected_mean_ms(route);
        TEST_ASSERT_FLOAT_WITHIN_MESSAGE(expected_ms * MEAN_TOLERANCE, expected_ms,
                                         total_ms / SAMPLE_COUNT, route->path);
    }
}

void test_get_service_ms_range(void) {
    route_info route  = make_test_route(10, 0);
    unsigned int seed = TEST_SEED;

    for (int i = 0; i < SAMPLE_COUNT; i++) {
        double service_ms = get_service_ms(&route, &seed);

        TEST_ASSERT_TRUE(service_ms >= 90.0);
        TEST_ASSERT_TRUE(service_ms <= 110.0);
    }
}

void test_get_service_ms_same_seed(void) {
    route_info route    = make_test_route(10, 10);
    unsigned int seed_a = TEST_SEED;
    unsigned int seed_b = TEST_SEED;

    for (int i = 0; i < SEQUENCE_LENGTH; i++) {
        // Unity's float macros evaluate their arguments more than once
        double first_ms  = get_service_ms(&route, &seed_a);
        double second_ms = get_service_ms(&route, &seed_b);

        TEST_ASSERT_EQUAL_FLOAT(first_ms, second_ms);
    }
}

void test_get_service_ms_consecutive(void) {
    route_info route  = make_test_route(10, 0);
    unsigned int seed = TEST_SEED;
    double first_ms   = get_service_ms(&route, &seed);
    int changed_count = 0;

    for (int i = 0; i < SEQUENCE_LENGTH; i++) {
        if (get_service_ms(&route, &seed) != first_ms) {
            changed_count++;
        }
    }
    TEST_ASSERT_GREATER_THAN_INT(0, changed_count);
}

void test_get_service_ms_no_jitter(void) {
    route_info route  = make_test_route(0, 0);
    unsigned int seed = TEST_SEED;

    for (int i = 0; i < SEQUENCE_LENGTH; i++) {
        TEST_ASSERT_EQUAL_FLOAT(100.0, get_service_ms(&route, &seed));
    }
}

void test_get_service_ms_long_pct_10(void) {
    route_info route  = make_test_route(0, 10);
    unsigned int seed = TEST_SEED;

    // 8% to 12% of samples
    TEST_ASSERT_INT_WITHIN(SAMPLE_COUNT / 50, SAMPLE_COUNT / 10, count_long_samples(&route, &seed));
}

void test_get_service_ms_long_pct_0(void) {
    route_info route  = make_test_route(0, 0);
    unsigned int seed = TEST_SEED;

    TEST_ASSERT_EQUAL_INT(0, count_long_samples(&route, &seed));
}

void test_get_service_ms_long_pct_100(void) {
    route_info route  = make_test_route(0, 100);
    unsigned int seed = TEST_SEED;

    TEST_ASSERT_EQUAL_INT(SAMPLE_COUNT, count_long_samples(&route, &seed));
}

void test_service_route_invalid(void) {
    unsigned int seed = TEST_SEED;

    TEST_ASSERT_EQUAL_INT(ERROR, service_route(ROUTE_UNKNOWN, &seed));
    TEST_ASSERT_EQUAL_INT(ERROR, service_route(ROUTE_COUNT, &seed));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_get_service_ms_average);
    RUN_TEST(test_get_service_ms_range);
    RUN_TEST(test_get_service_ms_same_seed);
    RUN_TEST(test_get_service_ms_consecutive);
    RUN_TEST(test_get_service_ms_no_jitter);
    RUN_TEST(test_get_service_ms_long_pct_10);
    RUN_TEST(test_get_service_ms_long_pct_0);
    RUN_TEST(test_get_service_ms_long_pct_100);
    RUN_TEST(test_service_route_invalid);
    return UNITY_END();
}
