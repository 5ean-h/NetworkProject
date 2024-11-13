#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <limits.h>
#include "TCPServerUtility.h"
#include "DieWithMessage.h"

int maxlives = 12;
char *word[] = {
    #include "words"
};
#define NUM_OF_WORDS (sizeof(word) / sizeof(word[0]))
#define MAXLEN 80               /* Maximum size of any string */
#define HANGMAN_TCP_PORT 5066
#define MAX_CLIENTS FD_SETSIZE

#define BUFFER 256

typedef struct {
    int fd;
    char part_word[MAXLEN];
    char *whole_word;
    int lives;
    int game_state;
    int word_length;
} client_t;

fd_set allset;

void client(client_t *client, int fd);
void remove_client(client_t *clients, int i);
void play_hangman(client_t *client);

int main() {
	int serverSock = SetupTCPServerSocket("5066");
    //int listen_fd, connection_fd;
    //socklen_t client_len;
    //struct sockaddr_in server, client_addr;
    fd_set rset;
    client_t clients[MAX_CLIENTS];
    int i;

    //srand((int)time((long *)0)); /* Randomize the seed */

    //listen_fd = socket(AF_INET, SOCK_STREAM, 0);

    //server.sin_family = AF_INET;
    //server.sin_addr.s_addr = htonl(INADDR_ANY);
    //server.sin_port = htons(HANGMAN_TCP_PORT);

    //bind(listen_fd, (struct sockaddr *)&server, sizeof(server));
    //listen(listen_fd, 5);

    for (i = 0; i < MAX_CLIENTS; i++)
        clients[i].fd = -1;

    FD_ZERO(&allset);
    FD_SET(serverSock, &allset);

    while (1) {
        rset = allset;
        select(FD_SETSIZE, &rset, NULL, NULL, NULL);

        if (FD_ISSET(serverSock, &rset)) {
            //client_len = sizeof(client_addr);
            //connection_fd = accept(serverSock, (struct sockaddr *)&client_addr, &client_len);
			
			struct sockaddr_storage clientAddr;
			socklen_t addrLen = sizeof(clientAddr);
			int clientSock = accept(serverSock, (struct sockaddr *)&clientAddr, &addrLen);
			if (clientSock < 0) {
                perror("accept() failed");
                continue;
            }
			
            for (i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].fd < 0) {
                    // Select a random word for the client
                    clients[i].fd = clientSock;
                    clients[i].whole_word = word[rand() % NUM_OF_WORDS];
                    clients[i].word_length = strlen(clients[i].whole_word);
                    clients[i].lives = maxlives;
                    clients[i].game_state = 'I'; // Incomplete game

                    // Send the selected word to the client
                    char outbuf[MAXLEN];
                    snprintf(outbuf, sizeof(outbuf), "%s\n", clients[i].whole_word);
                    write(clientSock, outbuf, strlen(outbuf));

                    // Initialize part_word with dashes
                    memset(clients[i].part_word, '-', clients[i].word_length);
                    clients[i].part_word[clients[i].word_length] = '\0';
					
                    break;
                }
            }

            if (i == MAX_CLIENTS) {
                close(clientSock);
                continue;
            }

            FD_SET(clientSock, &allset);
        }

        /* Check clients */
        for (i = 0; i < MAX_CLIENTS; i++) {
            int sockfd = clients[i].fd;
            if (sockfd < 0)
                continue;
            if (FD_ISSET(sockfd, &rset)) {
                play_hangman(&clients[i]);

                if (clients[i].game_state != 'I') {
                    /* Game over */
                    remove_client(clients, i);
                }
            }
        }
    }
	return 0;
}

void initialize_client(client_t *client, int fd) {
    char outbuf[MAXLEN];

    client->fd = fd;
    client->lives = maxlives;
    client->game_state = 'I'; // Incomplete

    // Select a random word for the client
    client->whole_word = word[rand() % NUM_OF_WORDS];
    int word_length = strlen(client->whole_word);

    // Send the selected word to the client
    snprintf(outbuf, sizeof(outbuf), "%s\n", client->whole_word);
    write(fd, outbuf, strlen(outbuf));

    // Initialize part_word with dashes
    memset(client->part_word, '-', word_length);
    client->part_word[word_length] = '\0';
}

 /* ---------------- Play_hangman () ---------------------*/
void play_hangman(client_t *client) {
    char guess[MAXLEN], outbuf[BUFFER];
    int word, i, good_guess;

    word = read(client->fd, guess, MAXLEN);
    if (word <= 0) {
        client->game_state = 'Q'; /* User quit */
        return;
    }

    guess[word - 1] = '\0';

    good_guess = 0;
    for (i = 0; i < client->word_length; i++) {
        if (guess[0] == client->whole_word[i]) {
            if (client->part_word[i] == '-') {
                good_guess = 1;
                client->part_word[i] = client->whole_word[i];
            }
        }
    }
    if (!good_guess)
        client->lives--;
    if (strcmp(client->whole_word, client->part_word) == 0)
        client->game_state = 'W'; /* User won */
    else if (client->lives == 0) {
        client->game_state = 'L'; /* User lost */
        strcpy(client->part_word, client->whole_word); /* Show the word */
    }

    snprintf(outbuf, sizeof(outbuf), "%s %d\n", client->part_word, client->lives);
    write(client->fd, outbuf, strlen(outbuf));
}
