#include <routes.h>

static const route_info ROUTE_TABLE[ROUTE_COUNT] = {
    // route_id: route_name, route_kind, ms, jitter, (bimodal low ms), (bimodal high ms)
    [ROUTE_FAST] = { "/fast", ROUTE_KIND_CPU, 1, 10, 0, 0 },
    [ROUTE_SLOW] = { "/slow", ROUTE_KIND_CPU, 500, 10, 0, 0 },
    [ROUTE_BIMODAL] = { "/bimodal", ROUTE_KIND_CPU, 1, 10, 10, 100 },
    [ROUTE_API] = { "/api", ROUTE_KIND_SLEEP, 20, 10, 0, 0 },
    [ROUTE_DB] = { "/db", ROUTE_KIND_SLEEP, 10, 50, 0, 0 },
};

const route_info *route_get(route_id id) {
    if (id < 0 || id >= ROUTE_COUNT) {
        return NULL;
    }
    return &ROUTE_TABLE[id];
}

route_id route_from_path(const char *path) {
    for (route_id id = 0; id < ROUTE_COUNT; id++) {
        if (strcmp(path, ROUTE_TABLE[id].path) == 0) {
            return id;
        }
    }
    return ROUTE_UNKNOWN;
}