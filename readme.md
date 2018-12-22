# Movie Search Engine

## Use:

* Parses and indexes CSV files containing movie titles
* allows for multiple client connections to a multi-server
* searches for movies based on titles


* Makefile
* queryclient.c: pulled the client-server comm to a different functions
* queryclient.h: reflects changes to queryclient.c
* queryprotocol.h: had to modify definitions of const char*s so they can
be used in multiple files.
* queryprotocol.o: reflects changes in queryprotocol.h.
