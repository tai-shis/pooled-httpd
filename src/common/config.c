#include "include/config.h"
#include <string.h>

/**
 * @brief Initializes the config to default values.
 * 
 * @param cfg Pointer to the server config.
 */
void set_config_defaults(config *cfg) {
    // Ensure config has nothing in it (eliminates junk data for unfilled positions)
    memset(cfg, 0, sizeof *cfg);

    // server info defaults
    cfg->port = 8080;
    cfg->num_threads = 4;
    cfg->queue_capacity = 64;
    cfg->policy = QUEUE_FIFO;
    cfg->model = MODEL_POOL;

    // non-FIFO policy defaults
    cfg->alpha = 0.5;
    cfg->w = 1.0;
    cfg->seed = 1;

    // timeouts - 5000ms (5s) default
    cfg->rcv_timeout_ms = 5000;
    cfg->snd_timeout_ms = 5000;

    // logging defaults
    cfg->log_path = "server.log";
    cfg->quiet = 0; // Maybe this should be FALSE constant?
}


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
void print_config(const config *cfg, FILE *out) {
    fprintf(out, "config:\n");
    fprintf(out, "\tport           = %d\n", cfg->port);
    fprintf(out, "\tthreads        = %d\n", cfg->num_threads);
    fprintf(out, "\tqueue capacity = %d\n", (int) cfg->queue_capacity);
    fprintf(out, "\tpolicy         = %s\n", cfg->policy == QUEUE_FIFO ? "FIFO" : "priority");
    fprintf(out, "\tmodel          = %s\n", cfg->model == MODEL_POOL ? "pool" : "thread-per-request");

    fprintf(out, "\talpha          = %.2f\n", cfg->alpha);
    fprintf(out, "\tw              = %.2f\n", cfg->w);
    fprintf(out, "\tseed           = %d\n", cfg->seed);

    fprintf(out, "\trcv_timeout_ms = %d\n", cfg->rcv_timeout_ms);
    fprintf(out, "\tsnd_timeout_ms = %d\n", cfg->snd_timeout_ms);
    
    fprintf(out, "\tlog_path       = %s\n", cfg->log_path ? cfg->log_path : "no log path");
    fprintf(out, "\tquiet          = %d\n", cfg->quiet);
}