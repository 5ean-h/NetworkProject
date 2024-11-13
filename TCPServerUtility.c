#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "TCPServerUtility.h"
#include "DieWithMessage.h"

int SetupTCPServerSocket(const char *port) {
    struct addrinfo hints, *serverInfo;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;       // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags = AI_PASSIVE;     // Any local IP

    if (getaddrinfo(NULL, port, &hints, &serverInfo) != 0)
        DieWithMessage("getaddrinfo() failed");

    int serverSock = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);
    if (serverSock < 0)
        DieWithMessage("socket() failed");

    if (bind(serverSock, serverInfo->ai_addr, serverInfo->ai_addrlen) < 0)
        DieWithMessage("bind() failed");

    if (listen(serverSock, SOMAXCONN) < 0)
        DieWithMessage("listen() failed");

    freeaddrinfo(serverInfo);
    return serverSock;
}
