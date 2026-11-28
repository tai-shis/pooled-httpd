#include "unity.h"
#include "service_time.h"

static service_time *s;

void setUp(void) {
    s = service_time_create(0.5);
}
void tearDown(void) {
    service_time_destroy(s);
}

static void test_starts_at_zero(void) {
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, service_time_get(s, ROUTE_FAST));
}

static void test_first_sample_sets_tau_directly(void) {
    service_time_update(s, ROUTE_FAST, 10.0);
    /* Without seeding, alpha*10 + (1-alpha)*0 would give 5.0 instead */
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 10.0, service_time_get(s, ROUTE_FAST));
}

static void test_known_samples_alpha_half(void) {
    service_time_update(s, ROUTE_FAST, 10.0); /* tau = 10 */
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 10.0, service_time_get(s, ROUTE_FAST));

    service_time_update(s, ROUTE_FAST, 20.0); /* 0.5*20 + 0.5*10 = 15 */
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 15.0, service_time_get(s, ROUTE_FAST));

    service_time_update(s, ROUTE_FAST, 30.0); /* 0.5*30 + 0.5*15 = 22.5 */
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 22.5, service_time_get(s, ROUTE_FAST));
}

static void test_routes_are_independent(void) {
    service_time_update(s, ROUTE_FAST, 10.0);
    service_time_update(s, ROUTE_SLOW, 100.0);
    service_time_update(s, ROUTE_SLOW, 200.0); /* 150 */

    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 10.0, service_time_get(s, ROUTE_FAST));
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 150.0, service_time_get(s, ROUTE_SLOW));
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, service_time_get(s, ROUTE_MEDIUM));
}

static void test_each_route_seeds_separately(void) {
    service_time_update(s, ROUTE_FAST, 10.0);
    service_time_update(s, ROUTE_SLOW, 80.0); /* first SLOW sample: must be 80, not blended */
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 80.0, service_time_get(s, ROUTE_SLOW));
}

static void test_invalid_input_is_ignored(void) {
    service_time_update(s, ROUTE_UNKNOWN, 5.0);
    service_time_update(s, ROUTE_COUNT, 5.0);
    service_time_update(s, ROUTE_FAST, -1.0);
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, service_time_get(s, ROUTE_FAST));
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, service_time_get(s, ROUTE_UNKNOWN));
}

static void test_bad_alpha_rejected(void) {
    TEST_ASSERT_NULL(service_time_create(0.0));
    TEST_ASSERT_NULL(service_time_create(-0.5));
    TEST_ASSERT_NULL(service_time_create(1.5));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_starts_at_zero);
    RUN_TEST(test_first_sample_sets_tau_directly);
    RUN_TEST(test_known_samples_alpha_half);
    RUN_TEST(test_routes_are_independent);
    RUN_TEST(test_each_route_seeds_separately);
    RUN_TEST(test_invalid_input_is_ignored);
    RUN_TEST(test_bad_alpha_rejected);
    return UNITY_END();
}