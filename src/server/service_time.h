#ifndef SERVICE_TIME_H
#define SERVICE_TIME_H

#include "routes.h"

typedef struct service_time service_time; /* opaque */

/* alpha must be in (0, 1]. Returns NULL on bad alpha or allocation failure. */
service_time *service_time_create(double alpha);
void          service_time_destroy(service_time *s);

/* Worker: record one wall-clock service time (seconds) for a route.
 * Ignores invalid routes and negative samples. */
void service_time_update(service_time *s, route_id route, double t);

/* Acceptor: current estimate (seconds). 0.0 if the route has no samples yet
 * or the route is invalid. */
double service_time_get(service_time *s, route_id route);

#endif