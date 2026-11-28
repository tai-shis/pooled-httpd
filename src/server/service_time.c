#include "service_time.h"

#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>

struct service_time {
    pthread_mutex_t lock; /* its own lock, separate from the queue's */
    double          alpha;
    double          tau[ROUTE_COUNT];
    bool            seeded[ROUTE_COUNT];
};

static bool valid(route_id r) {
    return r >= 0 && r < ROUTE_COUNT;
}

service_time *service_time_create(double alpha) {
    if (!(alpha > 0.0 && alpha <= 1.0)) {
        return NULL;
    }

    service_time *s = calloc(1, sizeof *s); /* tau = 0, seeded = false */
    if (!s) {
        return NULL;
    }
    s->alpha = alpha;
    pthread_mutex_init(&s->lock, NULL);
    return s;
}

void service_time_destroy(service_time *s) {
    if (!s) {
        return;
    }
    pthread_mutex_destroy(&s->lock);
    free(s);
}

void service_time_update(service_time *s, route_id route, double t) {
    if (!valid(route) || t < 0.0) {
        return;
    }

    pthread_mutex_lock(&s->lock);
    if (!s->seeded[route]) {
        s->tau[route]    = t; /* first sample sets tau = t directly */
        s->seeded[route] = true;
    } else {
        s->tau[route] = s->alpha * t + (1.0 - s->alpha) * s->tau[route];
    }
    pthread_mutex_unlock(&s->lock);
}

double service_time_get(service_time *s, route_id route) {
    if (!valid(route)) {
        return 0.0;
    }

    pthread_mutex_lock(&s->lock);
    double v = s->tau[route];
    pthread_mutex_unlock(&s->lock);
    return v;
}