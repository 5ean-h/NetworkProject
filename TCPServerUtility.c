/* TCPServerUtility.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include "DieWithMessage.h"

int SetupTCPServerSocket(const char *port) {
    struct addrinfo hints, *res, *p;
    int server_sock;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, port, &hints, &res) != 0) DieWithMessage("getaddrinfo failed", NULL);

    for (p = res; p != NULL; p = p->ai_next) {
        server_sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (server_sock == -1) continue;
        if (bind(server_sock, p->ai_addr, p->ai_addrlen) == 0) break;
        close(server_sock);
    }

    if (p == NULL) DieWithMessage("bind failed", NULL);
    freeaddrinfo(res);

    if (listen(server_sock, 10) == -1) DieWithMessage("listen failed", NULL);

    return server_sock;
}