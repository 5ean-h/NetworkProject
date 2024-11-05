/* Network server for hangman game using select() */
/* File: hangserver.c */

 #include <sys/types.h>
 #include <sys/socket.h>
 #include <netinet/in.h>
 #include <stdio.h>
 #include <syslog.h>
 #include <errno.h>
 #include <stdlib.h>
 #include <unistd.h>
 #include <time.h>
 #include <string.h>
 #include <limits.h> 

 extern time_t time ();

 int maxlives = 12;
 char *word [] = {
     #include "words"
 };
 #define NUM_OF_WORDS (sizeof (word) / sizeof (word [0]))
 #define MAXLEN 80               /* Maximum size of any string */
 #define HANGMAN_TCP_PORT 5066
 #define MAX_CLIENTS FD_SETSIZE  

 #define Buffer 256

 typedef struct {
     int fd;
     char part_word [MAXLEN];  
     char *whole_word;         
     int lives;                
     int game_state;           
     int word_length;          
 } client_t;

 fd_set allset;

 void client (client_t *client, int fd);
 void remove_client (client_t *clients, int i, int *maxfd);
 void play_hangman (client_t *client);

 int main ()
 {
     int listen_fd, conn_fd, maxfd, nready;
     socklen_t client_len;
     struct sockaddr_in server, client_addr;
     fd_set rset;
     client_t clients [MAX_CLIENTS];
     int i, maxi;

     srand ((unsigned int) time (NULL)); /* Randomize the seed */

     listen_fd = socket (AF_INET, SOCK_STREAM, 0);
     if (listen_fd <0) {
         perror ("creating stream socket");
         exit (1);
     }

     int opt = 1;
     setsockopt (listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof (opt));

     server.sin_family = AF_INET;
     server.sin_addr.s_addr = htonl (INADDR_ANY);
     server.sin_port = htons (HANGMAN_TCP_PORT);

     if (bind (listen_fd, (struct sockaddr *) &server, sizeof (server)) <0) {
         perror ("binding socket");
         exit (2);
     }

     if (listen (listen_fd, 5) <0) {
         perror ("listen");
         exit (3);
     }

     /* client array */
     for (i = 0; i < MAX_CLIENTS; i++)
         clients [i].fd = -1;

     FD_ZERO (&allset);
     FD_SET (listen_fd, &allset);
     maxfd = listen_fd;
     maxi = -1;

     while (1) {
         rset = allset;
         nready = select (maxfd + 1, &rset, NULL, NULL, NULL);
         if (nready <0) {
             perror ("select error");
             exit (4);
         }

         if (FD_ISSET (listen_fd, &rset)) {
             client_len = sizeof (client_addr);
             if ((conn_fd = accept (listen_fd, (struct sockaddr *) &client_addr, &client_len)) <0) {
                 perror ("accept error");
                 continue;
             }

             for (i = 0; i < MAX_CLIENTS; i++) {
                 if (clients [i].fd <0) {
                     client (&clients [i], conn_fd);
                     break;
                 }
             }

             if (i == MAX_CLIENTS) {
                 fprintf (stderr, "Too many clients\n");
                 close (conn_fd);
                 continue;
             }

             FD_SET (conn_fd, &allset);
             if (conn_fd > maxfd)
                 maxfd = conn_fd;
             if (i > maxi)
                 maxi = i;

             if (--nready <= 0)
                 continue;
         }

         /*Check all clients*/
         for (i = 0; i <= maxi; i++) {
             int sockfd;
             if ((sockfd = clients [i].fd) <0)
                 continue;
             if (FD_ISSET (sockfd, &rset)) {
                 play_hangman (&clients [i]);

                 if (clients [i].game_state != 'I') {
                     /*Game over*/
                     remove_client (clients, i, &maxfd);
                     if (i == maxi) {
                         while (maxi >= 0 && clients [maxi].fd <0)
                             maxi--;
                     }
                 }

                 if (--nready <= 0)
                     break;
             }
         }
     }
 }

 /* make a new client just before playing the game */
 void client (client_t *client, int fd)
 {
     char buffer [BUFFER];
     char hostname [HOST_NAME_MAX + 1];

     client->fd = fd;
     client->lives = maxlives;
     client->game_state = 'I';

     /* Pick a word at random from the list */
     client->whole_word = word [rand () % NUM_OF_WORDS];
     client->word_length = strlen (client->whole_word);

     /* No letters are guessed initially */
     memset (client->part_word, '-', client->word_length);
     client->part_word [client->word_length] = '\0';

     gethostname (hostname, sizeof (hostname));
     snprintf (outbuf, sizeof (outbuf), "Playing hangman on host %s:\n\n", hostname);
     write (fd, outbuf, strlen (outbuf));

     syslog (LOG_USER | LOG_INFO, "server chose hangman word %s", client->whole_word);

     snprintf (outbuf, sizeof (outbuf), "%s %d\n", client->part_word, client->lives);
     write (fd, outbuf, strlen (outbuf));
 }

 void remove_client (client_t *clients, int i, int *maxfd)
 {
     close (clients [i].fd);
     FD_CLR (clients [i].fd, &allset);
     clients [i].fd = -1;

     if (clients [i].fd == *maxfd) {
         *maxfd = -1;
         for (int j = 0; j < MAX_CLIENTS; j++) {
             if (clients [j].fd > *maxfd)
                 *maxfd = clients [j].fd;
         }
     }
 }

 /* ---------------- Play_hangman () ---------------------*/
 void play_hangman (client_t *client)
 {
     char guess [MAXLEN], outbuf [Buffer];
     int n, i, good_guess;

     n = read (client->fd, guess, MAXLEN);
     if (n <= 0) {
         if (n <0 && errno == EINTR)
             return; 
         else {
             client->game_state = 'Q'; /* Q ==> Quit */
             return;
         }
     }

     guess [n - 1] = '\0';

     good_guess = 0;
     for (i = 0; i < client->word_length; i++) {
         if (guess [0] == client->whole_word [i]) {
             if (client->part_word [i] == '-') {
                 good_guess = 1;
                 client->part_word [i] = client->whole_word [i];
             }
         }
     }
     if (! good_guess)
         client->lives--;
     if (strcmp (client->whole_word, client->part_word) == 0)
         client->game_state = 'W'; /* W ==> User Won */
     else if (client->lives == 0) {
         client->game_state = 'L'; /* L ==> User Lost */
         strcpy (client->part_word, client->whole_word); /* User Show the word */
     }

     snprintf (outbuf, sizeof (outbuf), "%s %d\n", client->part_word, client->lives);
     write (client->fd, outbuf, strlen (outbuf));
 }

