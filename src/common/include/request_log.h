#ifndef REQUEST_LOG_H
#define REQUEST_LOG_H

#include "routes.h"

#include <stdbool.h>

typedef struct {
    int  log_fd;         // readonly after init, NO_FILE_LOGGING if no file logging
    bool copy_to_stderr; // readonly after init
} request_log;

/**
 * @brief Initializes a logger with the given file path and copy_to_stderr flag
 * @param log_path The path to the log file to open for writing, NULL for no file
 * @param copy_to_stderr If true, logs will also be copied to stderr
 * @return A pointer to the initialized logger, or NULL if initialization fails.
 */
request_log *initialize_request_log(const char *log_path, bool copy_to_stderr);

/**
 * @brief Frees the memory allocated for the logger as well as open fds
 *
 * @param request_log The logger to free
 */
void free_request_log(request_log *request_log);

/**
 * @brief Writes a log entry to the logger
 *
 * @param request_log The logger to write to
 * @param route_id The route id that was requested
 * @param queue_time The time spent in the queue before being serviced
 * @param service_time The time spent servicing the request
 * @return SUCCESS on success, ERROR on failure
 */
int log_request(const request_log *request_log, route_id route_id, double queue_time,
                double service_time);

#endif