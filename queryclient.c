#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "includes/queryprotocol.h"

struct addrinfo hints,
    *result;

void argValidator(int argc) {
  if (argc != 3) {
    printf("Incorrect number of arguments.\n");
    printf("Correct usage: ./queryclient [IP] [port]\n");
  }
}

void printQueryResults(int requests, int sock_fd) {
  while (1) {
    if (requests == 0) {
      break;
  }
  char response[1000];
  int didReceive = recv(sock_fd, response, 1000, 0);
  response[didReceive] = '\0';
  if (didReceive < 0) {
    perror("Did not receive anything from server\n");
    exit(1);
    }
    // printf("received %d bytes\n", didReceive);
  if (strcmp(response, "GOODBYE") == 0) {
     // printf("Goodbye sent from server, exiting connection\n\n");
    break;  // connection is still alive
    } else {  // printing response
      printf("%s\n", response);
      // printf("Sending Ack about results\n\n");
  if (SendAck(sock_fd) != 0) {
    perror("Ack was not sent successfully\n");
    exit(1);
      }
    }
  }
}


void RunQuery(char *query, char *ipAddress, char *portNumber) {
  memset(&hints, 0, sizeof(struct addrinfo));  // hints struct is empty
  hints.ai_family = AF_UNSPEC;                // either IPv4 or IPv6
  hints.ai_socktype = SOCK_STREAM;            // Using TCP sockets
  hints.ai_flags = AI_PASSIVE;                // fill in IP for me

  int status;
  int len;
  char response[1000];
  int sock_fd = socket(AF_INET, SOCK_STREAM, 0);  // socket creation;

  // local host = 127.0.0.1
  // Port = 1750 for example
  status = getaddrinfo(ipAddress, portNumber, &hints, &result);
  if (status != 0) {
    perror("couldn't get address info\n");
    exit(1);
  }

  // connecs to client

  if (connect(sock_fd, result->ai_addr, result->ai_addrlen) == -1) {
    perror("connection was not successful\n");
    exit(1);
  }
  len = recv(sock_fd, response, 1000, 0);
  response[len] = '\0';
  // Checks 1st ack from server after connect
  if (CheckAck(response) == -1) {
    perror("No acknowledgement received from server\n");
    exit(1);
  }

  // sending query to client
  if (send(sock_fd, query, strlen(query), 0) < 0) {
    perror("query was not sent succesfully to server\n");
    exit(1);
  }
  // Testing the multiserver
  // sleep(5);
  int requests = 0;  // TO-DO: convert to into or keep as a string?
  // number of responses received
  int index = recv(sock_fd, &requests, 1000, 0);
  response[index] = '\0';
  printf("\nConnected to a movie server\n");
  printf("\nReceived: %d search result(s): \n\n", requests);

  if (SendAck(sock_fd) != 0) {
    perror("Ack was not sent successfully\n");
    exit(1);
  }

  // printing results from query
  printQueryResults(requests, sock_fd);
  close(sock_fd);
  freeaddrinfo(result);
  // Find the address
  // Create the socket
  // Connect to the server
  // Do the query-protocol
  // Close the connection
}

void KillServer(char *ipAddress, char *portNumber) {
  memset(&hints, 0, sizeof(struct addrinfo));  // hints struct is empty
  hints.ai_family = AF_UNSPEC;                // either IPv4 or IPv6
  hints.ai_socktype = SOCK_STREAM;            // Using TCP sockets
  hints.ai_flags = AI_PASSIVE;                // fill in IP for me

  int status;
  int len;
  char response[1000];
  int sock_fd = socket(AF_INET, SOCK_STREAM, 0);  // socket creation;

  // local host = 127.0.0.1
  // Port = 1750 for example
  status = getaddrinfo(ipAddress, portNumber, &hints, &result);

  if (status != 0) {
    perror("couldn't get address info\n");
    exit(1);
  }

  // connecs to client

  if (connect(sock_fd, result->ai_addr, result->ai_addrlen) == -1) {
    perror("connection was not successful\n");
    exit(1);
  }
  len = recv(sock_fd, response, 1000, 0);  // bytes from recv
  response[len] = '\0';

  // Checks 1st ack from server after connect
  if (CheckAck(response) == -1) {
    perror("No acknowledgement received from server\n");
    exit(1);
  }

  SendKill(sock_fd);
  close(sock_fd);
  freeaddrinfo(result);
  }

  // Find the address
  // Create the socket
  // Connect to the server
  // Do the query-protocol
  // Close the connection


void RunPrompt(char *ipAddress, char *portNumber) {
  char input[1000];

  while (1) {
    printf("Enter a term to search for, q to quit or k to kill: ");
    scanf("%s", input);

    if (strlen(input) == 1) {
      if (input[0] == 'q') {
        // Quit the program
        return;
      } else {
    if ((input[0] == 'k') || input[0] == 'K') {
          KillServer(ipAddress, portNumber);
          return;
        }
      }
    }

    // Get the query
    RunQuery(input, ipAddress, portNumber);
  }
}

int main(int argc, char **argv) {
  argValidator(argc);
  RunPrompt(argv[1], argv[2]);
  // Check/get arguments
  // Get info from user
  // Run Query
  return 0;
}
