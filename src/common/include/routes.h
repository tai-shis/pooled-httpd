#ifndef ROUTES_H
#define ROUTES_H

typedef enum {
    ROUTE_FAST,
    ROUTE_SLOW,
    ROUTE_BIMODAL,
    ROUTE_API,
    ROUTE_DB,
    ROUTE_COUNT,
    ROUTE_UNKNOWN = -1
} route_id;

typedef enum {
    // spins on the CPU, so CPU contention shows up as latency
    ROUTE_KIND_CPU,
    // blocks in nanosleep and uses no CPU
    ROUTE_KIND_SLEEP
} route_kind;

typedef struct {
    // for example, "/fast" or "/api"
    const char *path;
    route_kind  kind;
    // typical service time, in ms
    unsigned mean_ms;
    // +/- variation, as a percentage of the chosen mean
    unsigned jitter_pct;
    // chance (0 to 100) that a request uses long_mean_ms instead of mean_ms
    unsigned long_pct;
    unsigned long_mean_ms;
} route_info;

/**
 * @brief looks up the parameters of a route
 * @param id the route to look up
 *
 * @return the route's parameters, or NULL if id is not a valid route.
 */
const route_info *route_get(route_id id);

/**
 * @brief finds the route whose path matches exactly
 * @param path the request path, for example "/fast"
 *
 * @return the matching route, or ROUTE_UNKNOWN if no route matches.
 */
route_id route_from_path(const char *path);

#endif
