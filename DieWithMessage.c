#include <stdio.h>
#include <stdlib.h>
#include "DieWithMessage.h"

void DieWithMessage(const char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}
