#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

#include "includes/queryprotocol.h"
#include "includes/docset.h"
#include "includes/movieIndex.h"
#include "includes/docidmap.h"
#include "Hashtable.h"
#include "includes/queryprocessor.h"
#include "includes/fileparser.h"
#include "includes/filecrawler.h"
#define SEARCH_RESULT_LENGTH 1500


int Cleanup();

DocIdMap docs;
Index docIndex;

char movieSearchResult[SEARCH_RESULT_LENGTH];

void argValidator(int argc) {
  if (argc != 3) {
    printf("Incorrect number of arguments.\n");
    printf("Correct usage: ./multiserver [datadir] [port]\n");
  }
}

void sigchld_handler(int s) {
  write(0, "Handling zombies...\n", 20);
  // waitpid() might overwrite errno, so we save and restore it:
  int saved_errno = errno;
  while (waitpid(-1, NULL, WNOHANG) > 0);
  errno = saved_errno;
}

void sigint_handler(int sig) {
  write(0, "Ahhh! SIGINT!\n", 14);
  Cleanup();
  exit(0);
}


void sendResultsToClient(SearchResultIter iter, int client_fd) {
  SearchResult output = malloc(sizeof(struct searchResult));
  char dest[1000] = {'\0'};
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


/**
 * 
 */
int HandleConnections(int sock_fd) {
    // Step 5: Accept connection
    // Connecting with client
    char query[1000];
    char response[1000];
    int client_fd;
    while ((client_fd = accept(sock_fd, NULL, NULL)) != -1) {
    printf("Connection made with client\n");

    // forking on the connection
    if (!fork()) {
      close(sock_fd);  // closing parent

    // sending connection acknowledgment
    if (SendAck(client_fd) == -1) {
      perror("Acknowledgement was not sent successfully\n");
    }

    // Getting query from client
    int len = read(client_fd, query, sizeof(query) - 1);
    query[len] = '\0';

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
    }
  }  // results! = 0
}
  close(client_fd);
  // Fork on every connection.
  return 0;
}

void Setup(char *dir) {
  struct sigaction sa;

    sa.sa_handler = sigchld_handler;  // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
      perror("sigaction");
      exit(1);
    }

    struct sigaction kill;

    kill.sa_handler = sigint_handler;
    kill.sa_flags = 0;  // or SA_RESTART
    sigemptyset(&kill.sa_mask);

    if (sigaction(SIGINT, &kill, NULL) == -1) {
      perror("sigaction");
      exit(1);
    }

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

  int main(int argc, char **argv) {
  // Get args
    argValidator(argc);
    Setup(argv[1]);

    // Step 1: Get address stuff
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
  // Step 5: Handle the connections
  HandleConnections(sock_fd);
  // Got Kill signal
    close(sock_fd);
    freeaddrinfo(result);
    Cleanup();

    return 0;
  }
