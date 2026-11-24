#include <stdio.h>
#include <stdlib.h>
#include "include/config.h"
#include "../constants.h"
#include "include/errprint.h"

int main(int argc, char *argv[]) {
    if (argc < 1) {
        printf("Incorrect usage:");
        exit(EXIT_FAILURE);
    }

    config cfg;
    int status = config_parse(&cfg, argc, argv);
    if (status != SUCCESS) {
        printerr(status);
        exit(EXIT_FAILURE);
    }

    print_config(&cfg, stderr);
    exit(EXIT_SUCCESS);
}