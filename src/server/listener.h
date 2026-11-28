#ifndef LISTENER_H
#define LISTENER_H

#include <signal.h>

// Creates a listening socket on port
// returns the listening fd or -1 on error
int listener_open(int port, int backlog);

// called once per accepted connection, callback owns fd
typedef void (*conn_handler)(int fd, void *ctx);

// accepts until *stop is nonzero, returns 0 on clean stop and -1 on error
int accept_loop(int listen_fd, volatile sig_atomic_t *stop, conn_handler handler, void *ctx);

#endif