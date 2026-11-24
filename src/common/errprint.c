#include "include/errprint.h"
#include "../constants.h"
#include <stdio.h>
#include <limits.h>

void printerr(int err) {
    switch (err){
        case INVALID_MODEL_TYPE:
            fprintf(stderr, "Error: %d - Model type is invalid. Valid types are: pool, tpr\n", err);
            break;
        case INVALID_QUEUE_TYPE:
            fprintf(stderr, "Error: %d - Queue type is invalid. Valid types are: fifo, priority\n", err);
            break;
        case THREAD_NUM_OOB:
            fprintf(stderr, "Error: %d - Thread number is out of bounds. [%d, %d\n", err, MIN_THREADS, MAX_THREADS);
            break;
        case QUEUE_CAP_OOB:
            fprintf(stderr, "Error: %d - Queue cap is out of bounds. [%d, %d]\n", err, 1, MAX_QUEUE);
            break;
        case PORT_RANGE_OOB:
            fprintf(stderr, "Error: %d - Port range is out of bounds. [%d, %d]\n", err, 1024, 65535);
            break;
        case ALPHA_VAL_OOB:
            fprintf(stderr, "Error: %d - Alpha value is out of bounds. [%.2f, %.2f]\n", err, 0.0, 1.0);
            break;
        case WEIGHT_VAL_OOB:
            fprintf(stderr, "Error: %d - Weight value is out of bounds. [%.2f, %.2f]\n", err, 0.0, 1.0);
            break;
        case SEED_VAL_OOB:
            fprintf(stderr, "Error: %d - Seed value is out of bounds. [%d, %d]\n", err, 0, INT_MAX);
            break;
        case RCV_WAIT_OOB:
            fprintf(stderr, "Error: %d - Receive wait time is out of bounds. [%dms, %dms]\n", err, MIN_WAIT, MAX_WAIT);
            break;
        case SND_WAIT_OOB:
            fprintf(stderr, "Error: %d - Send wait time is out of bounds. [%dms, %dms]\n", err, MIN_WAIT, MAX_WAIT);
            break;
        case LOGPATH_TOO_LONG:
            fprintf(stderr, "Error: %d - Logpath too long. Max length is: %d\n", err, PATH_MAX);
            break;
        default:
            fprintf(stderr, "Error: %d - Unspecified error...exiting\n", err);
    }
}