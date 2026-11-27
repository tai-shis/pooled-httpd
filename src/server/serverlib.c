#include "serverlib.h"
#include "constants.h"

#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <time.h>

#define ITERATIONS_PER_MS 200000 // tweak this as needed to estimate ms per iter

static double get_random_fraction(unsigned int *seed) {
    return rand_r(seed) / (double)RAND_MAX;
}

static int is_bimodal(const route_info *route) {
    return route->long_pct > 0;
}

static double get_mean_ms(const route_info *route, unsigned int *seed) {
    // long_pct chance to use long_mean_ms instead of mean_ms
    if (!is_bimodal(route)) {
        return route->mean_ms;
    }

    if (get_random_fraction(seed) < (route->long_pct / PERCENT)) {
        return route->long_mean_ms;
    }

    return route->mean_ms;
}

static double get_jitter(const double mean_ms, const unsigned int jitter_percentage,
                         unsigned int *seed) {
    int negative = rand_r(seed) % 2; // 0 or 1

    double jitter_range = mean_ms * (jitter_percentage / PERCENT);
    double jitter       = get_random_fraction(seed) * jitter_range;

    if (negative) {
        return -jitter;
    }

    return jitter;
}

double get_service_ms(const route_info *route, unsigned int *seed) {
    double mean_ms = get_mean_ms(route, seed);
    double jitter  = get_jitter(mean_ms, route->jitter_pct, seed);
    return mean_ms + jitter;
}

static void busy_wait(double ms) {
    uint64_t iterations = (uint64_t)(ms * ITERATIONS_PER_MS);

    for (volatile uint64_t i = 0; i < iterations; i++) {
        // zzz
    }
}

static int sleep_wait(double ms) {
    struct timespec ts;

    time_t seconds     = (time_t)(ms / 1000);                         // whole seconds, probably 0
    long   nanoseconds = (long)((ms - (seconds * 1000)) * NS_PER_MS); // remaining nanoseconds
    ts.tv_sec          = seconds;
    ts.tv_nsec         = nanoseconds;

    while (nanosleep(&ts, &ts)) {
        if (errno != EINTR) {
            return ERROR; // error other than interruption
        }
    }

    return SUCCESS;
}

int service_route(const route_id id, unsigned int *seed) {
    const route_info *route = route_get(id);

    if (route == NULL) {
        return ERROR;
    }

    double service_time_ms = get_service_ms(route, seed);

    switch (route->kind) {
    case ROUTE_KIND_CPU:
        busy_wait(service_time_ms);
        break;
    case ROUTE_KIND_SLEEP:
        if (sleep_wait(service_time_ms) != SUCCESS) {
            return ERROR;
        }
        break;
    default:
        return ERROR;
    }

    return SUCCESS;
}