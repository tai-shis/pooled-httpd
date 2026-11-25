#include "include/config.h"
#include "../constants.h"
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>

static int parse_long(const char *str, int low, int high, long *out_num) {
    char *end;
    errno = 0;
    long value = strtol(str, &end, 10);
    if (*end != '\0' || errno == ERANGE || end == str || value < low || value > high) {
        return ERROR;
    }
    // valid input so set it and return success
    *out_num = value;
    return 0;
}

static int parse_double(const char *str, double low, double high, double *out_num) {
    char *end;
    errno = 0;
    double value = strtod(str, &end);
    if (*end != '\0' || errno == ERANGE || end == str || !(value >= low && value <= high)) {
        return ERROR;
    }
    *out_num = value;
    return 0;
}

/**
 * TODO: This function is a bit gross. Not sure if it's possible to reduce
 * line count by a whole lot just because there are a lot of options.
 * Can work on fixing it though.
 */
static int set_config_options(int argc, char **argv, config *cfg, const struct option options[]) {
    optind = 0; // reset option index to 0
    int opt;
    long l_val;
    double d_val;
    while ((opt = getopt_long(argc, argv, "", options, NULL)) != -1) {
        switch (opt) {
            case 'm':
                if(strcmp(optarg, "pool") == 0) {
                    cfg->model = MODEL_POOL;
                } else if((strcmp(optarg, "tpr")) == 0) {
                    cfg->model = MODEL_THREAD_PER_REQUEST;
                } else {
                    return INVALID_MODEL_TYPE;
                }
                break;
            case 'p':
                if(strcmp(optarg, "fifo") == 0) {
                    cfg->policy = (queue_policy) QUEUE_FIFO;
                } else if((strcmp(optarg, "priority")) == 0) {
                    cfg->policy = (queue_policy) QUEUE_PRIORITY;
                } else {
                    return INVALID_QUEUE_TYPE;
                }
                break;
            case 't':
                if (parse_long(optarg, MIN_THREADS, MAX_THREADS, &l_val) == ERROR) {
                    return THREAD_NUM_OOB;
                }
                cfg->num_threads = (int) l_val;
                break;
            case 'c':
                if (parse_long(optarg, 1, MAX_QUEUE, &l_val) == ERROR) {
                    return QUEUE_CAP_OOB;
                }
                cfg->queue_capacity = (size_t) l_val;
                break;
            case 'P':
                if (parse_long(optarg, 1024, 65535, &l_val) == ERROR) {
                    return PORT_RANGE_OOB;
                }
                cfg->port = (int) l_val;
                break;
            case 'a':
                if (parse_double(optarg, 0.0, 1.0, &d_val) == ERROR) {
                    return ALPHA_VAL_OOB;
                }
                cfg->alpha = d_val;
                break;
            case 'w':
                if (parse_double(optarg, 0.0, 10.0, &d_val) == ERROR) {
                    return WEIGHT_VAL_OOB;
                }
                cfg->w = d_val;
                break;
            case 's':
                if (parse_long(optarg, 0, INT_MAX, &l_val) == ERROR) {
                    return SEED_VAL_OOB;
                }
                cfg->seed = (unsigned int) l_val;
                break;
            case 'R':
                if (parse_long(optarg, MIN_WAIT, MAX_WAIT, &l_val) == ERROR) {
                    return RCV_WAIT_OOB;
                }
                cfg->rcv_timeout_ms = (int) l_val;
                break;
            case 'S': 
                if (parse_long(optarg, MIN_WAIT, MAX_WAIT, &l_val) == ERROR) {
                    return SND_WAIT_OOB;
                }
                cfg->snd_timeout_ms = (int) l_val;
                break;
            case 'L':
                if (strlen(optarg) >= PATH_MAX) {
                    return LOGPATH_TOO_LONG;
                }
                cfg->log_path = optarg;
                break;
            case 'q':
                cfg->quiet = 1;
                break;
            default:
                // Junk input, invalid flags
                return MALFORMED_ARGS;
        }
    }

    if (optind < argc) {
        return MALFORMED_ARGS;
    }

    return SUCCESS;
}

/**
 * @brief Initializes the config to default values.
 * 
 * @param cfg Pointer to the server config.
 */
static void set_config_defaults(config *cfg) {
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

int config_parse(config *cfg, int argc, char **argv) {
    set_config_defaults(cfg);

    static const struct option options[] = {
        {"model",       required_argument, 0, 'm'},
        {"policy",      required_argument, 0, 'p'},
        {"threads",     required_argument, 0, 't'},
        {"capacity",    required_argument, 0, 'c'},
        {"port",        required_argument, 0, 'P'},
        {"alpha",       required_argument, 0, 'a'},
        {"w",           required_argument, 0, 'w'},
        {"seed",        required_argument, 0, 's'},
        {"receive",     required_argument, 0, 'R'},
        {"send",        required_argument, 0, 'S'},
        {"logpath",     required_argument, 0, 'L'},
        {"quiet",       no_argument,       0, 'q'},
        {0,             0,                 0,  0}};

    int status = set_config_options(argc, argv, cfg, options);
    return status;
}

void print_config(const config *cfg, FILE *out) {
    fprintf(out, "config:\n");
    fprintf(out, "\tport           = %d\n",   cfg->port);
    fprintf(out, "\tthreads        = %d\n",   cfg->num_threads);
    fprintf(out, "\tqueue capacity = %u\n",   (unsigned int) cfg->queue_capacity);
    fprintf(out, "\tpolicy         = %s\n",   cfg->policy == QUEUE_FIFO ? "FIFO" : "priority");
    fprintf(out, "\tmodel          = %s\n",   cfg->model == MODEL_POOL ? "pool" : "thread-per-request");
    fprintf(out, "\talpha          = %.2f\n", cfg->alpha);
    fprintf(out, "\tw              = %.2f\n", cfg->w);
    fprintf(out, "\tseed           = %d\n",   cfg->seed);
    fprintf(out, "\trcv_timeout_ms = %d\n",   cfg->rcv_timeout_ms);
    fprintf(out, "\tsnd_timeout_ms = %d\n",   cfg->snd_timeout_ms);
    fprintf(out, "\tlog_path       = %s\n",   cfg->log_path ? cfg->log_path : "no log path");
    fprintf(out, "\tquiet          = %d\n",   cfg->quiet);
}