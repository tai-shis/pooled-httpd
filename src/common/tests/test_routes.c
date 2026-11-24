#include "unity.h"

#include "routes.h"

static const char *const EXPECTED_PATHS[ROUTE_COUNT] = {
    [ROUTE_FAST] = "/fast", [ROUTE_SLOW] = "/slow", [ROUTE_BIMODAL] = "/bimodal",
    [ROUTE_API] = "/api",   [ROUTE_DB] = "/db",
};

void setUp(void) {
}

void tearDown(void) {
}

void test_route_get_negative(void) {
    TEST_ASSERT_NULL(route_get(ROUTE_UNKNOWN));
}

void test_route_get_valid(void) {
    for (route_id id = 0; id < ROUTE_COUNT; id++) {
        const route_info *route = route_get(id);

        TEST_ASSERT_NOT_NULL(route);
        TEST_ASSERT_EQUAL_STRING(EXPECTED_PATHS[id], route->path);
    }
}

void test_route_get_too_large(void) {
    TEST_ASSERT_NULL(route_get(ROUTE_COUNT));
    TEST_ASSERT_NULL(route_get((route_id)(ROUTE_COUNT + 5)));
}

void test_route_from_path_exists(void) {
    for (route_id id = 0; id < ROUTE_COUNT; id++) {
        TEST_ASSERT_EQUAL_INT(id, route_from_path(EXPECTED_PATHS[id]));
    }
}

void test_route_from_path_missing(void) {
    TEST_ASSERT_EQUAL_INT(ROUTE_UNKNOWN, route_from_path("/nope"));
    TEST_ASSERT_EQUAL_INT(ROUTE_UNKNOWN, route_from_path(""));
    TEST_ASSERT_EQUAL_INT(ROUTE_UNKNOWN, route_from_path("/fast/"));
    TEST_ASSERT_EQUAL_INT(ROUTE_UNKNOWN, route_from_path("/FAST"));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_route_get_negative);
    RUN_TEST(test_route_get_valid);
    RUN_TEST(test_route_get_too_large);
    RUN_TEST(test_route_from_path_exists);
    RUN_TEST(test_route_from_path_missing);
    return UNITY_END();
}
