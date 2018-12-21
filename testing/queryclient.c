#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "includes/queryprotocol.h"
#include "queryclient.h"


void RunQueryHelper(int out_fd, int in_fd, const char *query) {
  
  // Connect

}

void RunQuery(char *query) {
  int sock_fd; // Get the socket

  RunQueryHelper(sock_fd, sock_fd, query); 

  // Close the connection and wait for another query
  close(sock_fd);

}

void KillServer() {
  
}

void RunPrompt() {
  char input[1000];

    while (1) {
    printf("Enter a term to search for, q to quit or k to kill: ");
    scanf("%s", input);

    printf("input was: %s\n", input); 
    
    if (strlen(input) == 1) {
      if (input[0] == 'q') {
	printf("Thanks for playing! \n");
	return; 
      } else {
	if ((input[0] == 'k') || input[0] == 'K') {
	  KillServer();
	  return; 
	}
      }
      }
       
    printf("\n\n");
    
    RunQuery(input);
    
      }
}
