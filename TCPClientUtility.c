/* TCPClientUtility.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "DieWithMessage.h"

int SetupTCPClientSocket(const char *host, const char *port) {
    struct addrinfo hints, *res, *p;
    int client_sock;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, port, &hints, &res) != 0) DieWithMessage("getaddrinfo failed", NULL);

    for (p = res; p != NULL; p = p->ai_next) {
        client_sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (client_sock == -1) continue;
        if (connect(client_sock, p->ai_addr, p->ai_addrlen) == 0) break;
        close(client_sock);
    }

    if (p == NULL) DieWithMessage("connect failed", NULL);
    freeaddrinfo(res);

    return client_sock;
}