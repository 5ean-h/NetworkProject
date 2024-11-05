#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <limits.h>

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
    int listen_fd, connection_fd;
    socklen_t client_len;
    struct sockaddr_in server, client_addr;
    fd_set rset;
    client_t clients[MAX_CLIENTS];
    int i;

    srand((int)time((long *)0)); /* Randomize the seed */

    listen_fd = socket(AF_INET, SOCK_STREAM, 0);

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(HANGMAN_TCP_PORT);

    bind(listen_fd, (struct sockaddr *)&server, sizeof(server));
    listen(listen_fd, 5);

    for (i = 0; i < MAX_CLIENTS; i++)
        clients[i].fd = -1;

    FD_ZERO(&allset);
    FD_SET(listen_fd, &allset);

    while (1) {
        rset = allset;
        select(FD_SETSIZE, &rset, NULL, NULL, NULL);

        if (FD_ISSET(listen_fd, &rset)) {
            client_len = sizeof(client_addr);
            connection_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_len);

            for (i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].fd < 0) {
                    client(&clients[i], connection_fd);
                    break;
                }
            }

            if (i == MAX_CLIENTS) {
                close(connection_fd);
                continue;
            }

            FD_SET(connection_fd, &allset);
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
}

void client(client_t *client, int fd) {
    char outbuf[BUFFER];
    char hostname[HOST_NAME_MAX + 1];

    client->fd = fd;
    client->lives = maxlives;
    client->game_state = 'I';

    /* Pick a word at random from the list */
    client->whole_word = word[rand() % NUM_OF_WORDS];
    client->word_length = strlen(client->whole_word);

    /* No letters are guessed initially */
    memset(client->part_word, '-', client->word_length);
    client->part_word[client->word_length] = '\0';

    gethostname(hostname, sizeof(hostname));
    snprintf(outbuf, sizeof(outbuf), "Playing hangman on host %s:\n\n", hostname);
    write(fd, outbuf, strlen(outbuf));

    snprintf(outbuf, sizeof(outbuf), "%s %d\n", client->part_word, client->lives);
    write(fd, outbuf, strlen(outbuf));
}

void remove_client(client_t *clients, int i) {
    close(clients[i].fd);
    FD_CLR(clients[i].fd, &allset);
    clients[i].fd = -1;
}

/* Play hangman with the client */
void play_hangman(client_t *client) {
    char guess[MAXLEN], outbuf[BUFFER];
    int word, i, good_guess;

    word = read(client->fd, guess, MAXLEN);
    if (word <= 0) {
        client->game_state = 'Q'; /* Client quit */
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
        client->game_state = 'W'; /* Client won */
    else if (client->lives == 0) {
        client->game_state = 'L'; /* Client lost */
        strcpy(client->part_word, client->whole_word); /* Show the word */
    }

    snprintf(outbuf, sizeof(outbuf), "%s %d\n", client->part_word, client->lives);
    write(client->fd, outbuf, strlen(outbuf));
}
