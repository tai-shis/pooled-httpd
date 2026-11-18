#ifndef ROUTES_H
#define ROUTES_H

typedef enum {
    ROUTE_FAST,
    ROUTE_MEDIUM,
    ROUTE_SLOW,
    ROUTE_COUNT,
    ROUTE_UNKNOWN = -1
} route_id;

typedef struct {
    // for example, "/fast" or "/medium" or "/slow"
    const char *path;      
    // how fast the route is, in ms
    unsigned    mean_ms; 
    // +/- variation in ms
    unsigned    jitter_ms; 
} route_info;

// turns an ID into its parameters
const route_info *route_get(route_id id);
// parses an incoming request
route_id          route_from_path(const char *path);

#endif