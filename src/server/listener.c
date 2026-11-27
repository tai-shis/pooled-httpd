#include "listener.h"

//remove once added to main and include constants.h
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

int listener_open(int port, int backlog) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        return -1;
    }

    int one = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one) < 0) {
        perror("setsockopt(SO_REUSEADDR)");
        close(fd);
        return -1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port        = htons((unsigned short)port);

    if (bind(fd, (struct sockaddr *)&addr, sizeof addr) < 0) {
        perror("bind");
        close(fd);
        return -1;
    }
    if (listen(fd, backlog) < 0) {
        perror("listen");
        close(fd);
        return -1;
    }
    return fd;
}

static void backoff_ms(long ms) {
    struct timespec ts = { ms / 1000, (ms % 1000) * 1000000L };
    nanosleep(&ts, NULL);   /* an early wake from a signal is harmless here */
}

int accept_loop(int listen_fd, volatile sig_atomic_t *stop,
                conn_handler handler, void *ctx) {
    while (!*stop) {
        int cfd = accept(listen_fd, NULL, NULL);
        if (cfd >= 0) {
            handler(cfd, ctx);
            continue;
        }

        switch (errno) {
        case EINTR:                 /* signal arrived: loop re-checks *stop */
        case ECONNABORTED:          /* client gave up before we accepted */
            continue;
        case EMFILE:
        case ENFILE:                /* out of fds: don't spin, give workers time to close some */
            fprintf(stderr, "accept: out of file descriptors, backing off\n");
            backoff_ms(100);
            continue;
        default:
            perror("accept");
            return -1;
        }
    }
    return 0;
}