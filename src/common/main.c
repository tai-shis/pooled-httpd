#include <stdio.h>
#include <stdlib.h>
#include "include/config.h"
#include "../constants.h"

int main(int argc, char *argv[]) {
    if (argc < 1) {
        printf("Incorrect usage:");
        exit(EXIT_FAILURE);
    }

    config cfg;
    int status = config_parse(&cfg, argc, argv);
    if (status != SUCCESS) {
        fprintf(stderr, "Error: %d\n", status);
        exit(EXIT_FAILURE);
    }

    print_config(&cfg, stderr);
    exit(EXIT_SUCCESS);
}