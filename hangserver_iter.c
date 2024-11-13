 /* Network server for hangman game */
 /* File: hangserver.c */

 #include <stdio.h>
 #include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <syslog.h>
#include <signal.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include "TCPServerUtility.h"
#include "DieWithMessage.h"

 extern time_t time ();

 int maxlives = 12;
 extern char *word[];
 # define NUM_OF_WORDS (sizeof (word) / sizeof (word [0]))
 # define MAXLEN 80 /* Maximum size in the world of Any string */
# define HANGMAN_TCP_PORT "5060" 

void reap_dead_processes(int signum);
void play_hangman(int in, int out);

int main() 
{
	
    int serverSock = SetupTCPServerSocket("5066");
    signal(SIGCHLD, reap_dead_processes);

    while (1) 
	{
        struct sockaddr_storage clientAddr;
        socklen_t addrLen = sizeof(clientAddr);
        int clientSock = accept(serverSock, (struct sockaddr *)&clientAddr, &addrLen);
        if (clientSock < 0) 
		{
            perror("accept() failed");
            continue;
        }


		// Select a random word for the client
        char *selected_word = word[rand() % NUM_OF_WORDS];
        write(clientSock, selected_word, strlen(selected_word));
        write(clientSock, "\n", 1);

        pid_t pid = fork();
        if (pid == 0) //Child process
		{ 
            close(serverSock);
            play_hangman(clientSock, clientSock);
            close(clientSock);
            exit(0);
        } else //Parent process
		{ 
            close(clientSock);
        }
    }
    return 0;
}

void reap_dead_processes(int signum) {
    (void)signum;
    while (waitpid(-1, NULL, WNOHANG) > 0);
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
