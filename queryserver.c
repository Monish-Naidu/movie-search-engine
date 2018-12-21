#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#define SEARCH_RESULT_LENGTH 1500

#include "includes/queryprotocol.h"
#include "includes/docset.h"
#include "includes/movieIndex.h"
#include "includes/docidmap.h"
#include "includes/Hashtable.h"
#include "includes/queryprocessor.h"
#include "includes/fileparser.h"
#include "includes/filecrawler.h"

DocIdMap docs;
Index docIndex;

char movieSearchResult[SEARCH_RESULT_LENGTH];


void argValidator(int argc) {
  if (argc != 3) {
    printf("Incorrect number of arguments.\n");
    printf("Correct usage: ./queryserver [datadir] [port]\n");
  }
}

int Cleanup();

void Setup(char *dir);

void sigint_handler(int sig) {
  write(0, "Exit signal sent. Cleaning up...\n", 34);
  Cleanup();
  exit(0);
}

void Setup(char *dir) {
  printf("Crawling directory tree starting at: %s\n", dir);
  // Create a DocIdMap
  docs = CreateDocIdMap();
  CrawlFilesToMap(dir, docs);

  printf("Crawled %d files.\n", NumElemsInHashtable(docs));

  // Create the index
  docIndex = CreateIndex();

  // Index the files
  printf("Parsing and indexing files...\n");
  ParseTheFiles(docs, docIndex);
  printf("%d entries in the index.\n", NumElemsInHashtable(docIndex));
}

int Cleanup() {
  DestroyIndex(docIndex);
  DestroyDocIdMap(docs);
  return 0;
}

void sendResultsToClient(SearchResultIter iter, int client_fd) {
  SearchResult output = malloc(sizeof(struct searchResult));
  char dest[1000]=  {'\0'};
  char response[1000];
  while (SearchResultIterHasMore(iter) != 0) {
    if (GetNextSearchResult(iter, output) != 0) {
      break;
    }
    GetRowFromFile(output, docs, dest);
    if (send(client_fd, dest, sizeof(dest), 0) == -1) {
      perror("Could not send result\n");
      exit(1);
    }
    int val = recv(client_fd, response, 1000, 0);  // receiving ack from client
    response[val] = '\0';

    if (CheckAck(response) != 0) {
      perror("No acknowledgement sent from client\n");
      exit(1);
    }
  }
  free(output);
  DestroySearchResultIter(iter);
  // all results sent, sending goodbye to client
  if (SendGoodbye(client_fd) != 0) {
    perror("Goodbye not sent successfully\n");
    exit(1);
  }
}

int main(int argc, char **argv) {
  // Check args
  argValidator(argc);

  Setup(argv[1]);

  // Step 1: get address/port info to open
  struct addrinfo hints,
      *result;
  int status;

  memset(&hints, 0, sizeof(struct addrinfo));  // hints struct is empty
  hints.ai_family = AF_UNSPEC;                // either IPv4 or IPv6
  hints.ai_socktype = SOCK_STREAM;            // Using TCP sockets
  hints.ai_flags = AI_PASSIVE;                // fill in IP for me

  // local host = 127.0.0.1
  // Port = 1750 for example
  status = getaddrinfo("127.0.0.1", argv[2], &hints, &result);
  if (status != 0) {
    perror("couldn't get address info\n");
    exit(1);
  }

  char response[1000];
  char query[1000];

  // Step 2: Open socket

  int sock_fd = socket(AF_INET, SOCK_STREAM, 0);  // socket creation;
  // Step 3: Bind socket
  if (bind(sock_fd, result->ai_addr, result->ai_addrlen) != 0) {
    perror("bind()");
    exit(1);
  }

  // Step 4: Listen on the socket

  if (listen(sock_fd, 10) != 0) {
    perror("listen() not successful");
    exit(1);
  }
  printf("Waiting for connection...\n");
  // Step 5: Handle clients that connect
  int client_fd;
  while (1) {
    // Connecting with client
    client_fd = accept(sock_fd, NULL, NULL);
    printf("Connection made with client\n");
    // sending connection acknowledgment
    if (SendAck(client_fd) == -1) {
      perror("Acknowledgement was not sent successfully\n");
    }
    // Getting query from client
    int len = read(client_fd, query, sizeof(query) - 1);
    query[len] = '\0';
    // Check if query is a kill 'k' or 'K'
    if (CheckKill(query) == 0) {
      printf("Kill message sent, server closing\n");
      Cleanup();
      freeaddrinfo(result);
      close(client_fd);
      close(sock_fd);
      exit(1);
    }
    SearchResultIter iter = FindMovies(docIndex, query);
    if (iter == NULL) {
      printf("No search results\n");
    }
    // Sending # response to client
    int results = 0;
    if (iter != NULL) {
      results = NumResultsInIter(iter);
    }
    printf("results from query: %d\n\n", results);
    if (send(client_fd, &results, sizeof(results), 0) == -1) {
      perror("Could not send number of results\n");
      exit(1);
    }
    // Getting first acknowledgement from client
    int index = recv(client_fd, response, 1000, 0);
    response[index] = '\0';

    if (CheckAck(response) != 0) {
      perror("No acknowledgement sent from client\n");
      exit(1);
    }
    if (results != 0) {
      sendResultsToClient(iter, client_fd);
    }  // results! = 0
  }  // outer while loop bracket
  // Step 6: Close the socket
  Cleanup();
  freeaddrinfo(result);
  close(client_fd);
  close(sock_fd);
  // return 0;
}
