#include "unity.h"

#include "constants.h"
#include "request_log.h"

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define TEMP_DIR_TEMPLATE "/tmp/test_request_log_XXXXXX"
#define LOG_FILE_NAME "test.log"
#define TEST_PATH_MAX 256
#define READ_BUFFER_SIZE 4096
#define ROUTE_PATH_MAX 16
#define TEST_UMASK 022
#define EXPECTED_FILE_MODE 0644
#define CONCURRENT_THREADS 8
#define CALLS_PER_THREAD 1000

static char         temp_dir[sizeof(TEMP_DIR_TEMPLATE)];
static char         log_path[TEST_PATH_MAX];
static request_log *logger;
static int          saved_stderr     = -1;
static int          stderr_pipe_read = -1;

typedef struct {
    const request_log *logger;
    int                thread_index;
    int                failures;
} concurrent_args;

static void redirect_stderr_to_pipe(void) {
    int pipe_fds[2];
    TEST_ASSERT_EQUAL_INT(SUCCESS, pipe(pipe_fds));
    TEST_ASSERT_NOT_EQUAL(ERROR, fcntl(pipe_fds[0], F_SETFL, O_NONBLOCK));

    saved_stderr = dup(STDERR_FILENO);
    TEST_ASSERT_NOT_EQUAL(ERROR, saved_stderr);
    TEST_ASSERT_NOT_EQUAL(ERROR, dup2(pipe_fds[1], STDERR_FILENO));

    close(pipe_fds[1]);
    stderr_pipe_read = pipe_fds[0];
}

static void restore_stderr(void) {
    if (saved_stderr != -1) {
        dup2(saved_stderr, STDERR_FILENO);
        close(saved_stderr);
        saved_stderr = -1;
    }

    if (stderr_pipe_read != -1) {
        close(stderr_pipe_read);
        stderr_pipe_read = -1;
    }
}

// returns the bytes waiting on the stderr pipe, or 0 if it is empty
static ssize_t read_stderr_pipe(char *buffer, size_t size) {
    ssize_t bytes_read = read(stderr_pipe_read, buffer, size - 1);

    if (bytes_read == ERROR) {
        TEST_ASSERT_EQUAL_INT(EAGAIN, errno);
        bytes_read = 0;
    }

    buffer[bytes_read] = '\0';
    return bytes_read;
}

static size_t read_file(const char *path, char *buffer, size_t size) {
    int fd = open(path, O_RDONLY);
    TEST_ASSERT_NOT_EQUAL(ERROR, fd);

    size_t  total = 0;
    ssize_t bytes_read;
    while ((bytes_read = read(fd, buffer + total, size - 1 - total)) > 0) {
        total += (size_t)bytes_read;
    }

    close(fd);
    TEST_ASSERT_NOT_EQUAL(ERROR, bytes_read);
    buffer[total] = '\0';
    return total;
}

static void write_file(const char *path, const char *contents) {
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, EXPECTED_FILE_MODE);
    TEST_ASSERT_NOT_EQUAL(ERROR, fd);
    ssize_t length = (ssize_t)strlen(contents);
    TEST_ASSERT_EQUAL_INT(length, write(fd, contents, (size_t)length));
    close(fd);
}

static void format_expected_line(char *buffer, size_t size, route_id route, double queue_time,
                                 double service_time) {
    snprintf(buffer, size, "route=%s wait_ms=%.3f service_ms=%.3f\n", route_get(route)->path,
             queue_time, service_time);
}

void setUp(void) {
    logger = NULL;
    snprintf(temp_dir, sizeof(temp_dir), "%s", TEMP_DIR_TEMPLATE);
    TEST_ASSERT_NOT_NULL(mkdtemp(temp_dir));
    snprintf(log_path, sizeof(log_path), "%s/%s", temp_dir, LOG_FILE_NAME);
}

void tearDown(void) {
    free_request_log(logger);
    logger = NULL;
    restore_stderr();
    unlink(log_path);
    rmdir(temp_dir);
}

void test_initialize_request_log_creates_file(void) {
    logger = initialize_request_log(log_path, false);

    TEST_ASSERT_NOT_NULL(logger);
    struct stat file_info;
    TEST_ASSERT_EQUAL_INT(SUCCESS, stat(log_path, &file_info));
    TEST_ASSERT_EQUAL_INT(0, file_info.st_size);
}

void test_initialize_request_log_truncates(void) {
    write_file(log_path, "old\n");

    logger = initialize_request_log(log_path, false);

    TEST_ASSERT_NOT_NULL(logger);
    struct stat file_info;
    TEST_ASSERT_EQUAL_INT(SUCCESS, stat(log_path, &file_info));
    TEST_ASSERT_EQUAL_INT(0, file_info.st_size);
}

void test_initialize_request_log_file_mode(void) {
    mode_t old_umask = umask(TEST_UMASK);
    logger           = initialize_request_log(log_path, false);
    umask(old_umask);

    TEST_ASSERT_NOT_NULL(logger);
    struct stat file_info;
    TEST_ASSERT_EQUAL_INT(SUCCESS, stat(log_path, &file_info));
    TEST_ASSERT_EQUAL_INT(EXPECTED_FILE_MODE, file_info.st_mode & 0777);
}

void test_initialize_request_log_null_path(void) {
    logger = initialize_request_log(NULL, false);

    TEST_ASSERT_NOT_NULL(logger);
    TEST_ASSERT_EQUAL_INT(NO_FILE_LOGGING, logger->log_fd);
}

void test_initialize_request_log_bad_path(void) {
    char missing_dir_path[TEST_PATH_MAX];
    snprintf(missing_dir_path, sizeof(missing_dir_path), "%s/missing/%s", temp_dir, LOG_FILE_NAME);

    errno  = 0;
    logger = initialize_request_log(missing_dir_path, false);

    TEST_ASSERT_NULL(logger);
    TEST_ASSERT_EQUAL_INT(ENOENT, errno);
}

void test_free_request_log_null(void) {
    free_request_log(NULL);
}

void test_free_request_log_closes_fd(void) {
    logger = initialize_request_log(log_path, false);
    TEST_ASSERT_NOT_NULL(logger);
    int log_fd = logger->log_fd;

    free_request_log(logger);
    logger = NULL;

    errno = 0;
    TEST_ASSERT_EQUAL_INT(ERROR, fcntl(log_fd, F_GETFD));
    TEST_ASSERT_EQUAL_INT(EBADF, errno);
}

void test_log_request_writes_line(void) {
    logger = initialize_request_log(log_path, false);
    TEST_ASSERT_NOT_NULL(logger);

    TEST_ASSERT_EQUAL_INT(SUCCESS, log_request(logger, ROUTE_FAST, 1.5, 2.25));

    char contents[READ_BUFFER_SIZE];
    read_file(log_path, contents, sizeof(contents));
    TEST_ASSERT_EQUAL_STRING("route=/fast wait_ms=1.500 service_ms=2.250\n", contents);
}

void test_log_request_rounding(void) {
    logger = initialize_request_log(log_path, false);
    TEST_ASSERT_NOT_NULL(logger);

    TEST_ASSERT_EQUAL_INT(SUCCESS, log_request(logger, ROUTE_SLOW, 1.23456, 0.0004));

    char contents[READ_BUFFER_SIZE];
    read_file(log_path, contents, sizeof(contents));
    TEST_ASSERT_EQUAL_STRING("route=/slow wait_ms=1.235 service_ms=0.000\n", contents);
}

void test_log_request_every_route(void) {
    for (route_id route = 0; route < ROUTE_COUNT; route++) {
        logger = initialize_request_log(log_path, false);
        TEST_ASSERT_NOT_NULL(logger);

        TEST_ASSERT_EQUAL_INT(SUCCESS, log_request(logger, route, 1.0, 2.0));

        char expected[READ_BUFFER_SIZE];
        char contents[READ_BUFFER_SIZE];
        format_expected_line(expected, sizeof(expected), route, 1.0, 2.0);
        read_file(log_path, contents, sizeof(contents));
        TEST_ASSERT_EQUAL_STRING(expected, contents);

        free_request_log(logger);
        logger = NULL;
    }
}

void test_log_request_appends_lines(void) {
    logger = initialize_request_log(log_path, false);
    TEST_ASSERT_NOT_NULL(logger);

    TEST_ASSERT_EQUAL_INT(SUCCESS, log_request(logger, ROUTE_FAST, 1.0, 2.0));
    TEST_ASSERT_EQUAL_INT(SUCCESS, log_request(logger, ROUTE_API, 3.0, 4.0));
    TEST_ASSERT_EQUAL_INT(SUCCESS, log_request(logger, ROUTE_DB, 5.0, 6.0));

    char contents[READ_BUFFER_SIZE];
    read_file(log_path, contents, sizeof(contents));
    TEST_ASSERT_EQUAL_STRING("route=/fast wait_ms=1.000 service_ms=2.000\n"
                             "route=/api wait_ms=3.000 service_ms=4.000\n"
                             "route=/db wait_ms=5.000 service_ms=6.000\n",
                             contents);
}

void test_log_request_null_logger(void) {
    errno = 0;

    TEST_ASSERT_EQUAL_INT(ERROR, log_request(NULL, ROUTE_FAST, 1.0, 2.0));
    TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}

void test_log_request_invalid_route(void) {
    logger = initialize_request_log(log_path, false);
    TEST_ASSERT_NOT_NULL(logger);

    errno = 0;
    TEST_ASSERT_EQUAL_INT(ERROR, log_request(logger, ROUTE_UNKNOWN, 1.0, 2.0));
    TEST_ASSERT_EQUAL_INT(EINVAL, errno);

    errno = 0;
    TEST_ASSERT_EQUAL_INT(ERROR, log_request(logger, ROUTE_COUNT, 1.0, 2.0));
    TEST_ASSERT_EQUAL_INT(EINVAL, errno);

    char contents[READ_BUFFER_SIZE];
    TEST_ASSERT_EQUAL_INT(0, read_file(log_path, contents, sizeof(contents)));
}

void test_log_request_stderr_copy(void) {
    redirect_stderr_to_pipe();
    logger = initialize_request_log(log_path, true);
    TEST_ASSERT_NOT_NULL(logger);

    TEST_ASSERT_EQUAL_INT(SUCCESS, log_request(logger, ROUTE_FAST, 1.0, 2.0));

    const char *expected = "route=/fast wait_ms=1.000 service_ms=2.000\n";
    char        contents[READ_BUFFER_SIZE];
    read_file(log_path, contents, sizeof(contents));
    TEST_ASSERT_EQUAL_STRING(expected, contents);
    read_stderr_pipe(contents, sizeof(contents));
    TEST_ASSERT_EQUAL_STRING(expected, contents);
}

void test_log_request_quiet(void) {
    redirect_stderr_to_pipe();
    logger = initialize_request_log(log_path, false);
    TEST_ASSERT_NOT_NULL(logger);

    TEST_ASSERT_EQUAL_INT(SUCCESS, log_request(logger, ROUTE_FAST, 1.0, 2.0));

    char contents[READ_BUFFER_SIZE];
    read_file(log_path, contents, sizeof(contents));
    TEST_ASSERT_EQUAL_STRING("route=/fast wait_ms=1.000 service_ms=2.000\n", contents);
    TEST_ASSERT_EQUAL_INT(0, read_stderr_pipe(contents, sizeof(contents)));
}

void test_log_request_stderr_only(void) {
    redirect_stderr_to_pipe();
    logger = initialize_request_log(NULL, true);
    TEST_ASSERT_NOT_NULL(logger);

    TEST_ASSERT_EQUAL_INT(SUCCESS, log_request(logger, ROUTE_FAST, 1.0, 2.0));

    char contents[READ_BUFFER_SIZE];
    read_stderr_pipe(contents, sizeof(contents));
    TEST_ASSERT_EQUAL_STRING("route=/fast wait_ms=1.000 service_ms=2.000\n", contents);
}

void test_log_request_no_destinations(void) {
    redirect_stderr_to_pipe();
    logger = initialize_request_log(NULL, false);
    TEST_ASSERT_NOT_NULL(logger);

    TEST_ASSERT_EQUAL_INT(SUCCESS, log_request(logger, ROUTE_FAST, 1.0, 2.0));

    char contents[READ_BUFFER_SIZE];
    TEST_ASSERT_EQUAL_INT(0, read_stderr_pipe(contents, sizeof(contents)));
}

// /dev/full accepts open() but every write() fails with ENOSPC
void test_log_request_write_failure(void) {
    logger = initialize_request_log("/dev/full", false);
    TEST_ASSERT_NOT_NULL(logger);

    TEST_ASSERT_EQUAL_INT(ERROR, log_request(logger, ROUTE_FAST, 1.0, 2.0));
}

static void *log_from_thread(void *arg) {
    concurrent_args *args = arg;

    for (int call = 0; call < CALLS_PER_THREAD; call++) {
        if (log_request(args->logger, ROUTE_FAST, args->thread_index, call) != SUCCESS) {
            args->failures++;
        }
    }

    return NULL;
}

static void run_concurrent_loggers(concurrent_args args[]) {
    pthread_t threads[CONCURRENT_THREADS];

    for (int index = 0; index < CONCURRENT_THREADS; index++) {
        args[index] = (concurrent_args){logger, index, 0};
        TEST_ASSERT_EQUAL_INT(0,
                              pthread_create(&threads[index], NULL, log_from_thread, &args[index]));
    }

    for (int index = 0; index < CONCURRENT_THREADS; index++) {
        TEST_ASSERT_EQUAL_INT(0, pthread_join(threads[index], NULL));
    }
}

// returns the thread index the line came from, or ERROR if the line is malformed
static int get_line_thread_index(const char *line) {
    char   path[ROUTE_PATH_MAX];
    double queue_time;
    double service_time;

    if (sscanf(line, "route=%15s wait_ms=%lf service_ms=%lf", path, &queue_time, &service_time) !=
        3) {
        return ERROR;
    }

    char expected[READ_BUFFER_SIZE];
    format_expected_line(expected, sizeof(expected), ROUTE_FAST, queue_time, service_time);

    int thread_index = (int)queue_time;
    if (strcmp(expected, line) != 0 || thread_index < 0 || thread_index >= CONCURRENT_THREADS) {
        return ERROR;
    }

    return thread_index;
}

void test_log_request_concurrent(void) {
    logger = initialize_request_log(log_path, false);
    TEST_ASSERT_NOT_NULL(logger);

    concurrent_args args[CONCURRENT_THREADS];
    run_concurrent_loggers(args);

    for (int index = 0; index < CONCURRENT_THREADS; index++) {
        TEST_ASSERT_EQUAL_INT(0, args[index].failures);
    }

    int   lines_per_thread[CONCURRENT_THREADS] = {0};
    int   total_lines                          = 0;
    char  line[READ_BUFFER_SIZE];
    FILE *log_file = fopen(log_path, "r");
    TEST_ASSERT_NOT_NULL(log_file);

    while (fgets(line, sizeof(line), log_file) != NULL) {
        int thread_index = get_line_thread_index(line);
        TEST_ASSERT_NOT_EQUAL_MESSAGE(ERROR, thread_index, line);
        lines_per_thread[thread_index]++;
        total_lines++;
    }

    fclose(log_file);
    TEST_ASSERT_EQUAL_INT(CONCURRENT_THREADS * CALLS_PER_THREAD, total_lines);
    for (int index = 0; index < CONCURRENT_THREADS; index++) {
        TEST_ASSERT_EQUAL_INT(CALLS_PER_THREAD, lines_per_thread[index]);
    }
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_initialize_request_log_creates_file);
    RUN_TEST(test_initialize_request_log_truncates);
    RUN_TEST(test_initialize_request_log_file_mode);
    RUN_TEST(test_initialize_request_log_null_path);
    RUN_TEST(test_initialize_request_log_bad_path);
    RUN_TEST(test_free_request_log_null);
    RUN_TEST(test_free_request_log_closes_fd);
    RUN_TEST(test_log_request_writes_line);
    RUN_TEST(test_log_request_rounding);
    RUN_TEST(test_log_request_every_route);
    RUN_TEST(test_log_request_appends_lines);
    RUN_TEST(test_log_request_null_logger);
    RUN_TEST(test_log_request_invalid_route);
    RUN_TEST(test_log_request_stderr_copy);
    RUN_TEST(test_log_request_quiet);
    RUN_TEST(test_log_request_stderr_only);
    RUN_TEST(test_log_request_no_destinations);
    RUN_TEST(test_log_request_write_failure);
    RUN_TEST(test_log_request_concurrent);
    return UNITY_END();
}
