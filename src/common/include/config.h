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
    const char *host;
    int         rps;
    int         duration_s;
    int         connections;

    // policy info
    double       alpha;
    double          w;
    unsigned int seed; // seed to be given to rand_r 

    // timeouts - 0 = wait forever
      // when calling setsockopt() to configure socket options, convert this
      // to a timeval struct to pass in as option (optval) 
    int          rcv_timeout_ms; // sockets(7) SO_RCVTIMEO     
    int          snd_timeout_ms; // sockets(7) SO_SNDTIMEO

    // logging
    const char  *log_path; // NULL for no logfile
    int          quiet; // 1 for no per-request printout on stderr (0 default)

    // view info (TODO)
    // and others
} config;

/**
 * @brief Parses the arguments in argv into the given config.
 * 
 * @param cfg Config struct to fill
 * 
 * @return SUCCESS when config parsed, otherwise some error is returned
 */
int config_parse(config *cfg, int argc, char **argv);

/**
 * @brief Prints current config STDERR in the console.
 * 
 * @param cfg Pointer to the current config struct.
 * @param out Output stream for config print. This SHOULD be the STDERR.
 * 
 * NOTE: Beaware that NO checking is done to ensure that it is in fact the STDERR
 * stream.
 * 
 * NOTE: During testing this function makes it easy to pass output somewhere
 * else (e.g. temp file). During normal server runtime it will call
 * print_config(&cfg, stderr); 
 */
void print_config(const config *cfg, FILE *out);
#endif