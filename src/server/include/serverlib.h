#ifndef SERVERLIB_H
#define SERVERLIB_H

#include "routes.h"

/**
 * @brief Calculates the service time for a given route, including mean service time and jitter.
 * @param route Pointer to the route_info structure for the route.
 * @param seed Pointer to an unsigned int used for random number generation.
 * @return The calculated service time in milliseconds.
 */
double get_service_ms(const route_info *route, unsigned int *seed);

/**
 * @brief Simulates servicing a request for a given route.
 * @param id The route_id of the route to service.
 * @param seed Pointer to an unsigned int used for random number generation.
 * @return SUCCESS (0) if the service was successful, ERROR (-1) if there was an error, 
 *  setting ERRNO (EINVAL and EFAULT)
 */
int service_route(const route_id id, unsigned int *seed);

#endif