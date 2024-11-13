#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "TCPClientUtility.h"
#include "DieWithMessage.h"

int SetupTCPClientSocket(const char *host, const char *port) {
    struct addrinfo hints, *serverInfo;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;       // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP

    if (getaddrinfo(host, port, &hints, &serverInfo) != 0)
        DieWithMessage("getaddrinfo() failed");

    int clientSock = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);
    if (clientSock < 0)
        DieWithMessage("socket() failed");

    if (connect(clientSock, serverInfo->ai_addr, serverInfo->ai_addrlen) < 0)
        DieWithMessage("connect() failed");

    freeaddrinfo(serverInfo);
    return clientSock;
}
