/* Hangclient.c - Client for hangman server.  */

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

#define LINESIZE 80
#define HANGMAN_TCP_PORT 5066

int main(int argc, char *argv[]) {
    char *server_name = (argc == 2) ? argv[1] : "localhost";
    char *server_port = "5066";
    int sock = SetupTCPClientSocket(server_name, server_port);
    if (sock < 0) {
        fprintf(stderr, "Error: Could not establish connection to server\n");
        exit(1);
    }

	char word[LINESIZE];
    char i_line[LINESIZE], o_line[LINESIZE];
    int count;
	
	count = read(sock, word, LINESIZE);
    if (count <= 0) {
        fprintf(stderr, "Error: Failed to receive word from server\n");
        close(sock);
        exit(1);
    }
    word[count - 1] = '\0'; // Ensure null-terminated string
    printf("Word received from server: %s\n", word);
	
	
    printf("Connected to server %s\n", server_name);
    while ((count = read(sock, i_line, LINESIZE)) > 0) {
        write(1, i_line, count);
        count = read(0, o_line, LINESIZE); // 0 = STDIN
        write(sock, o_line, count);
    }

    close(sock);
    return 0;
}

