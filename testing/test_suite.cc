// CS 5007, Northeastern University, Seattle
// Summer 2017
//
// Final Project: Client/Server
#include "gtest/gtest.h"
#include <cstdlib>

extern "C" {
#include "includes/Hashtable.h"
#include "includes/LinkedList.h"
#include "includes/docidmap.h"
#include "includes/filecrawler.h"
#include "includes/fileparser.h"
#include "includes/movieIndex.h"
#include "includes/movie.h"
#include "queryclient.h"
#include "includes/queryprotocol.h"
#include <string.h>
#include <stdio.h>
}



TEST(QueryClient, RunPrompt) {
  // Set up data for this test
  const char *query = "FOO";
  int numResults = 5;
  // It's easier to use a string sometime
  char numResultsStr[6]; // Not going to return more than 100,000 results
  snprintf(numResultsStr, sizeof(numResultsStr), "%d", numResults);
  const char *results[numResults];

  results[0] = "result1";
  results[1] = "result2";
  results[2] = "foobar";
  results[3] = "cs5007";
  results[4] = "summer!";

  
  // Set up pipes for fake server
  int p1[2];
  int p2[2];
  pipe(p1);
  pipe(p2);

  //  p1[0] // Parent READ (client reads from server)
  //  p1[1] // Parent WRITE (client writes to server)
  //  p2[0] // Child READ (server reads from client)
  //  p2[1] // Child WRITE (server writes to client)

  // Fork process to fake the server
  if( !fork() ){
    // Child process, acting as server.

    char stuff[100] = {0};
    
    SendAck(p2[1]); 
    wait(NULL);

    read(p1[0], stuff, 99);
    EXPECT_EQ(strcmp(stuff, query), 0);
    
    memset(stuff, 0, 100); 
    write(p2[1], numResultsStr, strlen(numResultsStr));

    wait(NULL); 
    read(p1[0], stuff, 99);
    EXPECT_EQ(0, CheckAck(stuff));

    for (int i=0; i<numResults; i++) {
      // Send result
      write(p2[1], results[i], strlen(results[i]));
      wait(NULL);
      
      // Wait for ACK
      read(p1[0], stuff, 99);
      EXPECT_EQ(0, CheckAck(stuff));   
    }
    
     // Send Goodbye
     SendGoodbye(p2[1]); 
     
     close(p1[1]);
     close(p1[0]);
     close(p2[1]);
     close(p2[0]);
       
     exit(0); 
  }

  // Will hold data printed to the stdout by the client
  //  Eventually, we'll check it to ensure that the results
  //  get printed on stdout. 
  char testString[1000] = {0};
 
  // Send stdout to nothing
  freopen("/dev/null", "a", stdout);
  // Capture stdout in a string buffer
  setvbuf(stdout, testString, _IOFBF, BUFSIZ);

  // Call the important function in queryclient.c
  // Query here is foo. 
  RunQueryHelper(p1[1], p2[0], query);

  // Send stdout back to the terminal so
  //  we can see the results of the test. 
  freopen ("/dev/tty", "a", stdout);

  for (int i=0; i<numResults; i++) {
    EXPECT_TRUE(strstr(testString, results[i]) != NULL);
  }

  // Just checking to see what the client output to the user.
  // This string is tested in the previous loop, but
  //  sometimes good to know. 
  printf("Output: %s\n", testString); 
}


int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
