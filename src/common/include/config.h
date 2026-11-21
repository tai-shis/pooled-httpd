#ifndef CONFIG_H
#define CONFIG_H

#include <stddef.h>
#include <stdio.h>
#include "queue.h"
#include "model.h"

typedef struct {
    // server info
    int          port;
    int          num_threads;
    size_t       queue_capacity;
    queue_policy policy;
    server_model model;

    // loadgen info
    const char  *host;
    int          rps;
    int          duration_s;
    int          connections;

    // policy info
    double       alpha;
    double          w;
    unsigned int seed; // seed to be given to rand_r 

    // timeouts - 0 = wait forever
    int          rcv_timeout_ms; // read sockets(7) SO_RCVTIMEO     
    int          snd_timeout_ms; // read sockets(7) SO_SNDTIMEO

    // logging
    const char  *log_path; // NULL for no logfile
    int          quiet; // 1 for no per-request printout on stderr (0 default)

    // view info (TODO)
    // and others
} config;

void set_config_defaults(config *cfg);
// 0 ok, -1 error 
int  config_parse(config *cfg, int argc, char **argv);  
void print_config(const config *cfg, FILE *out);

#endif