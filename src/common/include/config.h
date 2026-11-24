#ifndef CONFIG_H
#define CONFIG_H

#include <stddef.h>
#include "queue.h"

typedef struct {
    // server info
    int          port;
    int          num_threads;
    size_t       queue_capacity;
    queue_policy policy;

    // loadgen info
    const char *host;
    int         rps;
    int         duration_s;
    int         connections;

    // view info (TODO)
    // and others
} config;

void config_defaults(config *cfg);
// 0 ok, -1 error
int config_parse(config *cfg, int argc, char **argv);

#endif