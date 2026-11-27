/*
 * Unit tests for the config module (config.c / config.h).
 *
 * Build (adjust paths to your layout), for example:
 *   gcc -Wall -Wextra -g -I include -I <unity dir> -I <dir with constants.h> \
 *       tests/test_config.c config.c <unity dir>/unity.c -lm -o test_config
 *
 * Limits and error codes come from the project's own macros (MIN_THREADS,
 * MAX_THREADS, MAX_QUEUE, MIN_WAIT, MAX_WAIT, and the *_OOB / INVALID_* /
 * MALFORMED_ARGS codes) so these tests follow the real values.
 */
#include "unity.h"
#include "config.h"
#include "../../constants.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Unity ignores double asserts unless built with UNITY_INCLUDE_DOUBLE, so compare by hand. */
#define ASSERT_NEAR(expected, actual)                                                              \
    TEST_ASSERT_TRUE_MESSAGE(fabs((expected) - (actual)) < 1e-9, "double values differ")

void setUp(void) {
}
void tearDown(void) {
}

/* ------------------------------------------------------------------ */
/* Helpers                                                            */
/* ------------------------------------------------------------------ */

/*
 * Build a fresh argv for every parse. getopt_long may permute argv, so a
 * shared array must never be reused between calls.
 */
static int parse_args(config *cfg, int n, const char *const *args) {
    char *argv[64];
    TEST_ASSERT_TRUE(n < 62);
    argv[0] = "server";
    for (int i = 0; i < n; i++) {
        argv[i + 1] = (char *)args[i];
    }
    argv[n + 1] = NULL;
    return config_parse(cfg, n + 1, argv);
}

#define ARGS(...) ((const char *const[]){__VA_ARGS__})
#define NARGS(...) ((int)(sizeof(ARGS(__VA_ARGS__)) / sizeof(const char *)))
#define PARSE(cfg, ...) parse_args((cfg), NARGS(__VA_ARGS__), ARGS(__VA_ARGS__))

/* Parse and expect a specific error code. */
#define EXPECT_ERR(code, ...)                                                                      \
    do {                                                                                           \
        config c_;                                                                                 \
        TEST_ASSERT_EQUAL_INT((code), PARSE(&c_, __VA_ARGS__));                                    \
    } while (0)

/* Parse and expect success. */
#define EXPECT_OK(cfgptr, ...) TEST_ASSERT_EQUAL_INT(SUCCESS, PARSE((cfgptr), __VA_ARGS__))

/* Number to string; a small ring of buffers so several calls can share one argv. */
static const char *num(long v) {
    static char bufs[8][32];
    static int  next = 0;
    char       *b    = bufs[next++ % 8];
    snprintf(b, 32, "%ld", v);
    return b;
}

static void assert_defaults(const config *cfg) {
    TEST_ASSERT_EQUAL_INT(8080, cfg->port);
    TEST_ASSERT_EQUAL_INT(4, cfg->num_threads);
    TEST_ASSERT_EQUAL_UINT((unsigned)64, (unsigned)cfg->queue_capacity);
    TEST_ASSERT_EQUAL_INT(QUEUE_FIFO, cfg->policy);
    TEST_ASSERT_EQUAL_INT(MODEL_POOL, cfg->model);
    ASSERT_NEAR(0.5, cfg->alpha);
    ASSERT_NEAR(1.0, cfg->w);
    TEST_ASSERT_EQUAL_UINT(1u, cfg->seed);
    TEST_ASSERT_EQUAL_INT(5000, cfg->rcv_timeout_ms);
    TEST_ASSERT_EQUAL_INT(5000, cfg->snd_timeout_ms);
    TEST_ASSERT_NOT_NULL(cfg->log_path);
    TEST_ASSERT_EQUAL_STRING("server.log", cfg->log_path);
    TEST_ASSERT_EQUAL_INT(0, cfg->quiet);
}

/* ------------------------------------------------------------------ */
/* Defaults                                                           */
/* ------------------------------------------------------------------ */

void test_defaults_with_no_flags(void) {
    config cfg;
    memset(&cfg, 0xAB, sizeof cfg); /* junk in, defaults out */
    TEST_ASSERT_EQUAL_INT(SUCCESS, parse_args(&cfg, 0, NULL));
    assert_defaults(&cfg);
}

void test_single_flag_leaves_other_fields_default(void) {
    config cfg;
    EXPECT_OK(&cfg, "--threads", "8");
    TEST_ASSERT_EQUAL_INT(8, cfg.num_threads);
    TEST_ASSERT_EQUAL_INT(8080, cfg.port);
    TEST_ASSERT_EQUAL_UINT((unsigned)64, (unsigned)cfg.queue_capacity);
    TEST_ASSERT_EQUAL_INT(QUEUE_FIFO, cfg.policy);
    TEST_ASSERT_EQUAL_INT(MODEL_POOL, cfg.model);
    TEST_ASSERT_EQUAL_INT(0, cfg.quiet);
    TEST_ASSERT_EQUAL_STRING("server.log", cfg.log_path);
}

/* ------------------------------------------------------------------ */
/* Each flag overrides its field                                      */
/* ------------------------------------------------------------------ */

void test_model_flag(void) {
    config cfg;
    EXPECT_OK(&cfg, "--model", "pool");
    TEST_ASSERT_EQUAL_INT(MODEL_POOL, cfg.model);
    EXPECT_OK(&cfg, "--model", "tpr");
    TEST_ASSERT_EQUAL_INT(MODEL_THREAD_PER_REQUEST, cfg.model);
}

void test_policy_flag(void) {
    config cfg;
    EXPECT_OK(&cfg, "--policy", "fifo");
    TEST_ASSERT_EQUAL_INT(QUEUE_FIFO, cfg.policy);
    EXPECT_OK(&cfg, "--policy", "priority");
    TEST_ASSERT_EQUAL_INT(QUEUE_PRIORITY, cfg.policy);
}

void test_threads_flag(void) {
    config cfg;
    EXPECT_OK(&cfg, "--threads", "12");
    TEST_ASSERT_EQUAL_INT(12, cfg.num_threads);
}

void test_capacity_flag(void) {
    config cfg;
    EXPECT_OK(&cfg, "--capacity", "128");
    TEST_ASSERT_EQUAL_UINT((unsigned)128, (unsigned)cfg.queue_capacity);
}

void test_port_flag(void) {
    config cfg;
    EXPECT_OK(&cfg, "--port", "9090");
    TEST_ASSERT_EQUAL_INT(9090, cfg.port);
}

void test_alpha_flag(void) {
    config cfg;
    EXPECT_OK(&cfg, "--alpha", "0.25");
    ASSERT_NEAR(0.25, cfg.alpha);
}

void test_w_flag(void) {
    config cfg;
    EXPECT_OK(&cfg, "--w", "1.5");
    ASSERT_NEAR(1.5, cfg.w);
}

void test_seed_flag(void) {
    config cfg;
    EXPECT_OK(&cfg, "--seed", "42");
    TEST_ASSERT_EQUAL_UINT(42u, cfg.seed);
}

void test_receive_timeout_flag(void) {
    config cfg;
    EXPECT_OK(&cfg, "--receive", "1500");
    TEST_ASSERT_EQUAL_INT(1500, cfg.rcv_timeout_ms);
    TEST_ASSERT_EQUAL_INT(5000, cfg.snd_timeout_ms); /* untouched */
}

void test_send_timeout_flag(void) {
    config cfg;
    EXPECT_OK(&cfg, "--send", "2500");
    TEST_ASSERT_EQUAL_INT(2500, cfg.snd_timeout_ms);
    TEST_ASSERT_EQUAL_INT(5000, cfg.rcv_timeout_ms); /* untouched */
}

void test_logpath_flag(void) {
    config cfg;
    EXPECT_OK(&cfg, "--logpath", "run1.log");
    TEST_ASSERT_EQUAL_STRING("run1.log", cfg.log_path);
}

void test_quiet_flag(void) {
    config cfg;
    EXPECT_OK(&cfg, "--quiet");
    TEST_ASSERT_EQUAL_INT(1, cfg.quiet);
}

void test_equals_syntax(void) {
    config cfg;
    EXPECT_OK(&cfg, "--threads=6", "--policy=priority", "--alpha=0.1");
    TEST_ASSERT_EQUAL_INT(6, cfg.num_threads);
    TEST_ASSERT_EQUAL_INT(QUEUE_PRIORITY, cfg.policy);
    ASSERT_NEAR(0.1, cfg.alpha);
}

void test_all_flags_together(void) {
    config cfg;
    EXPECT_OK(&cfg, "--model", "tpr", "--policy", "priority", "--threads", "7", "--capacity", "32",
              "--port", "5555", "--alpha", "0.9", "--w", "2.5", "--seed", "99", "--receive", "100",
              "--send", "200", "--logpath", "all.log", "--quiet");
    TEST_ASSERT_EQUAL_INT(MODEL_THREAD_PER_REQUEST, cfg.model);
    TEST_ASSERT_EQUAL_INT(QUEUE_PRIORITY, cfg.policy);
    TEST_ASSERT_EQUAL_INT(7, cfg.num_threads);
    TEST_ASSERT_EQUAL_UINT((unsigned)32, (unsigned)cfg.queue_capacity);
    TEST_ASSERT_EQUAL_INT(5555, cfg.port);
    ASSERT_NEAR(0.9, cfg.alpha);
    ASSERT_NEAR(2.5, cfg.w);
    TEST_ASSERT_EQUAL_UINT(99u, cfg.seed);
    TEST_ASSERT_EQUAL_INT(100, cfg.rcv_timeout_ms);
    TEST_ASSERT_EQUAL_INT(200, cfg.snd_timeout_ms);
    TEST_ASSERT_EQUAL_STRING("all.log", cfg.log_path);
    TEST_ASSERT_EQUAL_INT(1, cfg.quiet);
}

void test_flag_order_does_not_matter(void) {
    config a, b;
    EXPECT_OK(&a, "--quiet", "--threads", "3", "--port", "7000");
    EXPECT_OK(&b, "--port", "7000", "--threads", "3", "--quiet");
    TEST_ASSERT_EQUAL_INT(a.num_threads, b.num_threads);
    TEST_ASSERT_EQUAL_INT(a.port, b.port);
    TEST_ASSERT_EQUAL_INT(a.quiet, b.quiet);
}

/* ------------------------------------------------------------------ */
/* Boundary values that must be accepted (limits are inclusive)       */
/* ------------------------------------------------------------------ */

void test_threads_boundaries_accepted(void) {
    config cfg;
    EXPECT_OK(&cfg, "--threads", num(MIN_THREADS));
    TEST_ASSERT_EQUAL_INT(MIN_THREADS, cfg.num_threads);
    EXPECT_OK(&cfg, "--threads", num(MAX_THREADS));
    TEST_ASSERT_EQUAL_INT(MAX_THREADS, cfg.num_threads);
}

void test_capacity_boundaries_accepted(void) {
    config cfg;
    EXPECT_OK(&cfg, "--capacity", "1");
    TEST_ASSERT_EQUAL_UINT(1u, (unsigned)cfg.queue_capacity);
    EXPECT_OK(&cfg, "--capacity", num(MAX_QUEUE));
    TEST_ASSERT_EQUAL_UINT((unsigned)MAX_QUEUE, (unsigned)cfg.queue_capacity);
}

void test_port_boundaries_accepted(void) {
    config cfg;
    EXPECT_OK(&cfg, "--port", "1024");
    TEST_ASSERT_EQUAL_INT(1024, cfg.port);
    EXPECT_OK(&cfg, "--port", "65535");
    TEST_ASSERT_EQUAL_INT(65535, cfg.port);
}

void test_alpha_boundaries_accepted(void) {
    config cfg;
    EXPECT_OK(&cfg, "--alpha", "0");
    ASSERT_NEAR(0.0, cfg.alpha);
    EXPECT_OK(&cfg, "--alpha", "1");
    ASSERT_NEAR(1.0, cfg.alpha);
    EXPECT_OK(&cfg, "--alpha", "1.0");
    ASSERT_NEAR(1.0, cfg.alpha);
}

void test_w_boundaries_accepted(void) {
    config cfg;
    EXPECT_OK(&cfg, "--w", "0"); /* w = 0 is FIFO behaviour: a useful control */
    ASSERT_NEAR(0.0, cfg.w);
    EXPECT_OK(&cfg, "--w", "10");
    ASSERT_NEAR(10.0, cfg.w);
}

void test_seed_boundaries_accepted(void) {
    config cfg;
    EXPECT_OK(&cfg, "--seed", "0");
    TEST_ASSERT_EQUAL_UINT(0u, cfg.seed);
    EXPECT_OK(&cfg, "--seed", num(INT_MAX));
    TEST_ASSERT_EQUAL_UINT((unsigned)INT_MAX, cfg.seed);
}

void test_timeout_boundaries_accepted(void) {
    config cfg;
    EXPECT_OK(&cfg, "--receive", num(MIN_WAIT), "--send", num(MIN_WAIT));
    TEST_ASSERT_EQUAL_INT(MIN_WAIT, cfg.rcv_timeout_ms);
    TEST_ASSERT_EQUAL_INT(MIN_WAIT, cfg.snd_timeout_ms);
    EXPECT_OK(&cfg, "--receive", num(MAX_WAIT), "--send", num(MAX_WAIT));
    TEST_ASSERT_EQUAL_INT(MAX_WAIT, cfg.rcv_timeout_ms);
    TEST_ASSERT_EQUAL_INT(MAX_WAIT, cfg.snd_timeout_ms);
}

void test_logpath_length_boundary_accepted(void) {
    static char path[PATH_MAX];
    memset(path, 'a', PATH_MAX - 1);
    path[PATH_MAX - 1] = '\0'; /* length PATH_MAX - 1 */
    config cfg;
    EXPECT_OK(&cfg, "--logpath", path);
    TEST_ASSERT_EQUAL_STRING(path, cfg.log_path);
}

/* ------------------------------------------------------------------ */
/* Bad values must be rejected with the right error                   */
/* ------------------------------------------------------------------ */

void test_invalid_model_rejected(void) {
    EXPECT_ERR(INVALID_MODEL_TYPE, "--model", "bogus");
    EXPECT_ERR(INVALID_MODEL_TYPE, "--model", "");
    EXPECT_ERR(INVALID_MODEL_TYPE, "--model", "POOL");  /* case-sensitive */
    EXPECT_ERR(INVALID_MODEL_TYPE, "--model", "pool "); /* trailing space */
    EXPECT_ERR(INVALID_MODEL_TYPE, "--model", "thread-per-request");
}

void test_invalid_policy_rejected(void) {
    EXPECT_ERR(INVALID_QUEUE_TYPE, "--policy", "sjf");
    EXPECT_ERR(INVALID_QUEUE_TYPE, "--policy", "");
    EXPECT_ERR(INVALID_QUEUE_TYPE, "--policy", "FIFO");
    EXPECT_ERR(INVALID_QUEUE_TYPE, "--policy", "fifo2");
}

void test_threads_out_of_range_rejected(void) {
    EXPECT_ERR(THREAD_NUM_OOB, "--threads", num((long)MIN_THREADS - 1));
    EXPECT_ERR(THREAD_NUM_OOB, "--threads", num((long)MAX_THREADS + 1));
    EXPECT_ERR(THREAD_NUM_OOB, "--threads", "-1");
    EXPECT_ERR(THREAD_NUM_OOB, "--threads", "0");
}

void test_threads_not_a_number_rejected(void) {
    EXPECT_ERR(THREAD_NUM_OOB, "--threads", "abc");
    EXPECT_ERR(THREAD_NUM_OOB, "--threads", "");
    EXPECT_ERR(THREAD_NUM_OOB, "--threads", " ");
    EXPECT_ERR(THREAD_NUM_OOB, "--threads", "4x"); /* trailing junk */
    EXPECT_ERR(THREAD_NUM_OOB, "--threads", "x4");
    EXPECT_ERR(THREAD_NUM_OOB, "--threads", "4.5");
    EXPECT_ERR(THREAD_NUM_OOB, "--threads", "4 4");
    EXPECT_ERR(THREAD_NUM_OOB, "--threads", "99999999999999999999"); /* ERANGE */
}

void test_capacity_rejected(void) {
    EXPECT_ERR(QUEUE_CAP_OOB, "--capacity", "0");
    EXPECT_ERR(QUEUE_CAP_OOB, "--capacity", "-5");
    EXPECT_ERR(QUEUE_CAP_OOB, "--capacity", num((long)MAX_QUEUE + 1));
    EXPECT_ERR(QUEUE_CAP_OOB, "--capacity", "abc");
    EXPECT_ERR(QUEUE_CAP_OOB, "--capacity", "");
    EXPECT_ERR(QUEUE_CAP_OOB, "--capacity", "10k");
}

void test_port_rejected(void) {
    EXPECT_ERR(PORT_RANGE_OOB, "--port", "0");
    EXPECT_ERR(PORT_RANGE_OOB, "--port", "1023"); /* privileged port */
    EXPECT_ERR(PORT_RANGE_OOB, "--port", "65536");
    EXPECT_ERR(PORT_RANGE_OOB, "--port", "-1");
    EXPECT_ERR(PORT_RANGE_OOB, "--port", "http");
    EXPECT_ERR(PORT_RANGE_OOB, "--port", "");
    EXPECT_ERR(PORT_RANGE_OOB, "--port", "80.80");
}

void test_alpha_rejected(void) {
    EXPECT_ERR(ALPHA_VAL_OOB, "--alpha", "-0.1");
    EXPECT_ERR(ALPHA_VAL_OOB, "--alpha", "1.1");
    EXPECT_ERR(ALPHA_VAL_OOB, "--alpha", "2");
    EXPECT_ERR(ALPHA_VAL_OOB, "--alpha", "abc");
    EXPECT_ERR(ALPHA_VAL_OOB, "--alpha", "");
    EXPECT_ERR(ALPHA_VAL_OOB, "--alpha", "0.5x");
    EXPECT_ERR(ALPHA_VAL_OOB, "--alpha", "nan"); /* NaN passes naive < > checks */
    EXPECT_ERR(ALPHA_VAL_OOB, "--alpha", "inf");
    EXPECT_ERR(ALPHA_VAL_OOB, "--alpha", "-inf");
    EXPECT_ERR(ALPHA_VAL_OOB, "--alpha", "1e400"); /* ERANGE */
}

void test_w_rejected(void) {
    EXPECT_ERR(WEIGHT_VAL_OOB, "--w", "-0.1");
    EXPECT_ERR(WEIGHT_VAL_OOB, "--w", "10.1");
    EXPECT_ERR(WEIGHT_VAL_OOB, "--w", "abc");
    EXPECT_ERR(WEIGHT_VAL_OOB, "--w", "");
    EXPECT_ERR(WEIGHT_VAL_OOB, "--w", "nan");
    EXPECT_ERR(WEIGHT_VAL_OOB, "--w", "inf");
    EXPECT_ERR(WEIGHT_VAL_OOB, "--w", "1.5.2");
}

void test_seed_rejected(void) {
    EXPECT_ERR(SEED_VAL_OOB, "--seed", "-1");
    EXPECT_ERR(SEED_VAL_OOB, "--seed", num((long)INT_MAX + 1));
    EXPECT_ERR(SEED_VAL_OOB, "--seed", "abc");
    EXPECT_ERR(SEED_VAL_OOB, "--seed", "");
    EXPECT_ERR(SEED_VAL_OOB, "--seed", "1.5");
    EXPECT_ERR(SEED_VAL_OOB, "--seed", "99999999999999999999");
}

void test_receive_timeout_rejected(void) {
    EXPECT_ERR(RCV_WAIT_OOB, "--receive", num((long)MIN_WAIT - 1));
    EXPECT_ERR(RCV_WAIT_OOB, "--receive", num((long)MAX_WAIT + 1));
    EXPECT_ERR(RCV_WAIT_OOB, "--receive", "abc");
    EXPECT_ERR(RCV_WAIT_OOB, "--receive", "");
    EXPECT_ERR(RCV_WAIT_OOB, "--receive", "5s");
}

void test_send_timeout_rejected(void) {
    EXPECT_ERR(SND_WAIT_OOB, "--send", num((long)MIN_WAIT - 1));
    EXPECT_ERR(SND_WAIT_OOB, "--send", num((long)MAX_WAIT + 1));
    EXPECT_ERR(SND_WAIT_OOB, "--send", "abc");
    EXPECT_ERR(SND_WAIT_OOB, "--send", "");
    EXPECT_ERR(SND_WAIT_OOB, "--send", "5s");
}

void test_logpath_too_long_rejected(void) {
    static char path[PATH_MAX + 1];
    memset(path, 'a', PATH_MAX);
    path[PATH_MAX] = '\0'; /* length PATH_MAX: too long */
    EXPECT_ERR(LOGPATH_TOO_LONG, "--logpath", path);
}

void test_unknown_flag_rejected(void) {
    EXPECT_ERR(MALFORMED_ARGS, "--bogus");
    EXPECT_ERR(MALFORMED_ARGS, "--bogus", "1");
    EXPECT_ERR(MALFORMED_ARGS, "--threads", "4", "--bogus"); /* valid flag, then unknown */
}

void test_missing_value_rejected(void) {
    EXPECT_ERR(MALFORMED_ARGS, "--threads"); /* nothing after the flag */
    EXPECT_ERR(MALFORMED_ARGS, "--policy");
    EXPECT_ERR(MALFORMED_ARGS, "--alpha");
    EXPECT_ERR(MALFORMED_ARGS, "--logpath");
}

void test_leftover_positional_args_rejected(void) {
    EXPECT_ERR(MALFORMED_ARGS, "pool");
    EXPECT_ERR(MALFORMED_ARGS, "--quiet", "extra");
    EXPECT_ERR(MALFORMED_ARGS, "--threads", "4", "8"); /* stray second number */
    EXPECT_ERR(MALFORMED_ARGS, "extra", "--quiet");    /* getopt permutes; still leftover */
}

/* ------------------------------------------------------------------ */

int main(void) {
    UNITY_BEGIN();

    /* defaults */
    RUN_TEST(test_defaults_with_no_flags);
    RUN_TEST(test_single_flag_leaves_other_fields_default);

    /* each flag */
    RUN_TEST(test_model_flag);
    RUN_TEST(test_policy_flag);
    RUN_TEST(test_threads_flag);
    RUN_TEST(test_capacity_flag);
    RUN_TEST(test_port_flag);
    RUN_TEST(test_alpha_flag);
    RUN_TEST(test_w_flag);
    RUN_TEST(test_seed_flag);
    RUN_TEST(test_receive_timeout_flag);
    RUN_TEST(test_send_timeout_flag);
    RUN_TEST(test_logpath_flag);
    RUN_TEST(test_quiet_flag);
    RUN_TEST(test_equals_syntax);
    RUN_TEST(test_all_flags_together);
    RUN_TEST(test_flag_order_does_not_matter);

    /* boundaries accepted */
    RUN_TEST(test_threads_boundaries_accepted);
    RUN_TEST(test_capacity_boundaries_accepted);
    RUN_TEST(test_port_boundaries_accepted);
    RUN_TEST(test_alpha_boundaries_accepted);
    RUN_TEST(test_w_boundaries_accepted);
    RUN_TEST(test_seed_boundaries_accepted);
    RUN_TEST(test_timeout_boundaries_accepted);
    RUN_TEST(test_logpath_length_boundary_accepted);

    /* rejected values */
    RUN_TEST(test_invalid_model_rejected);
    RUN_TEST(test_invalid_policy_rejected);
    RUN_TEST(test_threads_out_of_range_rejected);
    RUN_TEST(test_threads_not_a_number_rejected);
    RUN_TEST(test_capacity_rejected);
    RUN_TEST(test_port_rejected);
    RUN_TEST(test_alpha_rejected);
    RUN_TEST(test_w_rejected);
    RUN_TEST(test_seed_rejected);
    RUN_TEST(test_receive_timeout_rejected);
    RUN_TEST(test_send_timeout_rejected);
    RUN_TEST(test_logpath_too_long_rejected);

    /* rejected command lines */
    RUN_TEST(test_unknown_flag_rejected);
    RUN_TEST(test_missing_value_rejected);
    RUN_TEST(test_leftover_positional_args_rejected);

    return UNITY_END();
}