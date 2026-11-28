#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "listener.h"

static volatile sig_atomic_t g_stop = 0;

static void on_signal(int sig) {
    (void)sig;
    g_stop = 1;
}

static void drop_conn(int fd, void *ctx) {
    (void)ctx;
    const char *msg = "HTTP/1.0 200 OK\r\nContent-Length: 3\r\n\r\nok\n";
    write(fd, msg, strlen(msg));
    close(fd);
}

int main(void) {
    struct sigaction sa = {.sa_handler = on_signal}; /* no SA_RESTART on purpose */
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    int lfd = listener_open(8080, 128);
    if (lfd < 0) {
        return 1;
    }

    int rc = accept_loop(lfd, &g_stop, drop_conn, NULL);
    close(lfd);
    return rc == 0 ? 0 : 1;
}