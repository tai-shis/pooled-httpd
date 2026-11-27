#include "request_log.h"
#include "constants.h"

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

request_log *initialize_request_log(const char *log_path, bool copy_to_stderr) {
    request_log *logger = malloc(sizeof(request_log));

    if (logger == NULL) {
        return NULL;
    }

    int log_fd = NO_FILE_LOGGING;

    if (log_path != NULL) {
        log_fd = open(log_path, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, LOG_FILE_MODE);

        if (log_fd == ERROR) {
            free(logger);
            return NULL;
        }
    }

    logger->log_fd         = log_fd;
    logger->copy_to_stderr = copy_to_stderr;

    return logger;
}

void free_request_log(request_log *request_log) {
    if (request_log == NULL) {
        return;
    }

    if (request_log->log_fd != NO_FILE_LOGGING) {
        close(request_log->log_fd);
    }

    free(request_log);
}

static int write_log_entry(const int fd, const char *buffer, int line_length) {
    if (fd == NO_FILE_LOGGING || buffer == NULL || line_length <= 0) {
        errno = EINVAL;
        return ERROR;
    }

    ssize_t bytes_written;
    do {
        bytes_written = write(fd, buffer, line_length);
    } while (bytes_written == ERROR && errno == EINTR);

    if (bytes_written != line_length) {
        return ERROR;
    }

    return SUCCESS;
}

int log_request(const request_log *request_log, route_id route_id, double queue_time,
                double service_time) {
    if (request_log == NULL) {
        errno = EINVAL;
        return ERROR;
    }

    const route_info *route = route_get(route_id);

    if (route == NULL) {
        errno = EINVAL;
        return ERROR;
    }

    char buffer[MAX_LOG_BUFFER];

    int line_length = snprintf(buffer, sizeof(buffer), "route=%s wait_ms=%.3f service_ms=%.3f\n",
                               route->path, queue_time, service_time);

    if (line_length < 0 || line_length >= MAX_LOG_BUFFER) {
        return ERROR;
    }

    if (request_log->log_fd != NO_FILE_LOGGING) {
        if (write_log_entry(request_log->log_fd, buffer, line_length) == ERROR) {
            return ERROR;
        }
    }

    if (request_log->copy_to_stderr) {
        if (write_log_entry(STDERR_FILENO, buffer, line_length) == ERROR) {
            return ERROR;
        }
    }

    return SUCCESS;
}