/* DieWithMessage.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

void DieWithMessage(const char *msg, const char *detail) {
    if (detail) {
        fprintf(stderr, "%s: %s\n", msg, detail);
    } else {
        perror(msg);
    }
    exit(EXIT_FAILURE);
}