/* Hangclient.c - Client for hangman server.  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include "DieWithMessage.h"
#include "TCPClientUtility.h"

#define LINESIZE 80
#define HANGMAN_TCP_PORT "5066"

void DieWithMessage(const char* msg, const char* detail) {
    if (detail) {
        fprintf(stderr, "%s: %s\n", msg, detail);
    }
    else {
        perror(msg);
    }
    exit(EXIT_FAILURE);
}

int main(int argc, char* argv[])
{
    struct addrinfo hints, * server_info, * p;
    int sock, rv;
    char i_line[LINESIZE], o_line[LINESIZE];
    char* server_name;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <Server Name>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    server_name = argv[1];

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;     // Allow IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP socket

    if ((rv = getaddrinfo(server_name, HANGMAN_TCP_PORT, &hints, &server_info)) != 0) {
        DieWithMessage("getaddrinfo() failed", gai_strerror(rv));
    }


    for (p = server_info; p != NULL; p = p->ai_next) {
        if ((sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            continue;
        }

        if (connect(sock, p->ai_addr, p->ai_addrlen) == -1) {
            close(sock);
            continue;
        }

        break;
    }

    if (p == NULL) {
        DieWithMessage("Failed to connect", NULL);
    }

    freeaddrinfo(server_info);

    printf("Connected to the server.\n");

    while (1) {
        printf("Enter your guess: ");
        if (fgets(i_line, LINESIZE, stdin) == NULL) {
            perror("fgets");
            break;
        }
    
        i_line[strcspn(i_line, "\n")] = 0;

        if (write(sock, i_line, strlen(i_line)) == -1) {
            perror("write");
            break;
        }

        int bytes_read = read(sock, o_line, LINESIZE - 1);
        if (bytes_read == -1) {
            perror("read");
            break;
        }
        else if (bytes_read == 0) {
            printf("Server disconnected.\n");
            break;
        }
        
        o_line[bytes_read] = 0;
        printf("Server response: %s\n", o_line);

    }


    close(sock);
    return 0;

}
