 /* Network server for hangman game */
 /* File: hangserver.c */

 #include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <syslog.h>
#include <signal.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>

 extern time_t time ();

 int maxlives = 12;
 char *word [] = {
 # include "words"
 };
 # define NUM_OF_WORDS (sizeof (word) / sizeof (word [0]))
 # define MAXLEN 80 /* Maximum size in the world of Any string */
# define HANGMAN_TCP_PORT "8080" /*Updated port number for getaddrinfo*/


 main ()
 {
    struct addrinfo hints, * resource; /*Added for getaddrinfo*/
 	int sock, fd, client_len;
 	struct sockaddr_in client;

 	srand ((int) time ((long *) 0)); /* randomize the seed */

    /* Configure server information with getaddrinfo */
    printf("Configuring server...");
    memset(&hints, 0, sizeof(struct addrinfo)); /* Prepare hints */
    hints.ai_family = AF_INET;                  /* IPv4 */
    hints.ai_socktype = SOCK_STREAM;            /* TCP */
    hints.ai_flags = AI_PASSIVE;                /* Use server’s IP address (INADDR_ANY) */

    /* Fill the resource structure */
    int r = getaddrinfo(NULL, HANGMAN_TCP_PORT, &hints, &resource);
    if (r != 0) {
        perror("Failed");
        exit(1);
    }
    puts("done");

    sock = socket(resource->ai_family, resource->ai_socktype, resource->ai_protocol);
 	if (sock <0) { //This error checking is the code Stevens wraps in his Socket Function etc
 		perror ("creating stream socket");
        freeaddrinfo(resource);  // Free memory if socket creation fails
 		exit (1);
 	}

 	if (bind(sock, resource->ai_addr, resource->ai_addrlen) < 0) {
 		perror ("binding socket");
        close(sock);
        freeaddrinfo(resource);  // Free memory if bind fails
	 	exit (2);
 	}

    /* Free the resource allocated by getaddrinfo */
    freeaddrinfo(resource);

 	listen (sock, 5);

    /* Set up a signal handler to reap zombie child processes */
    signal(SIGCHLD, reap_dead_processes);

 	while (1) {
 		client_len = sizeof (client);
 		if ((fd = accept (sock, (struct sockaddr *) &client, &client_len)) <0) {
 			perror ("accepting connection");
 			continue;
 		}
 		
        /* Fork a new process to handle the client */
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork failed");
            close(fd);
            continue;
        }

        if (pid == 0) { /* Child process */
            close(sock);  
            play_hangman(fd, fd);  
            close(fd);  
            exit(0);  // Exit child process after handling client
        }
        else { /* Parent process */
            close(fd);  // Close client socket in parent process
        }
 	}
 }

 /* Signal handler to reap zombie processes */
 void reap_dead_processes(int signum) {
     (void)signum;  // Suppress unused parameter warning
     while (waitpid(-1, NULL, WNOHANG) > 0);  // Reap all dead child processes
 }

 /* ---------------- Play_hangman () ---------------------*/

 play_hangman (int in, int out)
 {
 	char * whole_word, part_word [MAXLEN],
 	guess[MAXLEN], outbuf [MAXLEN];

 	int lives = maxlives;
 	int game_state = 'I';//I = Incomplete
 	int i, good_guess, word_length;
 	char hostname[MAXLEN];

 	gethostname (hostname, MAXLEN);
 	sprintf(outbuf, "Playing hangman on host% s: \n \n", hostname);
 	write(out, outbuf, strlen (outbuf));

 	/* Pick a word at random from the list */
 	whole_word = word[rand() % NUM_OF_WORDS];
 	word_length = strlen(whole_word);
 	syslog (LOG_USER | LOG_INFO, "server chose hangman word %s", whole_word);

 	/* No letters are guessed Initially */
 	for (i = 0; i <word_length; i++)
 		part_word[i]='-';
 	
	part_word[i] = '\0';

 	sprintf (outbuf, "%s %d \n", part_word, lives);
 	write (out, outbuf, strlen(outbuf));

 	while (game_state == 'I')
 	/* Get a letter from player guess */
 	{
		while (read (in, guess, MAXLEN) <0) {
 			if (errno != EINTR)
 				exit (4);
 			printf ("re-read the startin \n");
 			} /* Re-start read () if interrupted by signal */
 	good_guess = 0;
 	for (i = 0; i <word_length; i++) {
 		if (guess [0] == whole_word [i]) {
 		good_guess = 1;
 		part_word [i] = whole_word [i];
 		}
 	}
 	if (! good_guess) lives--;
 	if (strcmp (whole_word, part_word) == 0)
 		game_state = 'W'; /* W ==> User Won */
 	else if (lives == 0) {
 		game_state = 'L'; /* L ==> User Lost */
 		strcpy (part_word, whole_word); /* User Show the word */
 	}
 	sprintf (outbuf, "%s %d \n", part_word, lives);
 	write (out, outbuf, strlen (outbuf));
 	}
 }
