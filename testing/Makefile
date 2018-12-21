# define the commands we'll use for compilation and library building
AR = ar
ARFLAGS = rcs
CC = gcc

GOOGLE_TEST_INCLUDE=${HOME}/src/googletest-release-1.8.0/googletest/include/
GTEST_DIR = ${HOME}/src/googletest-release-1.8.0/googletest

# define useful flags to cc/ld/etc.
CPPFLAGS += -isystem $(GTEST_DIR)/include
CXXFLAGS += -g -Wall -Wextra -pthread
CFLAGS = -g -Wall -I. -I.. -Iincludes -pthread
LDFLAGS = -L. 

#define common dependencies
OBJS = libs/docset.o libs/docidmap.o libs/fileparser.o libs/filecrawler.o \
	libs/movieIndex.o libs/LinkedList.o libs/Hashtable.o libs/movie.o \
	libs/queryprocessor.o libs/queryprotocol.o

HEADERS = fileparser.h filecrawler.h docidmap.h movieIndex.h docset.h \
	movie.h Hashtable.h LinkedList.h queryprotocol.h

GTEST_HEADERS = $(GTEST_DIR)/include/gtest/*.h \
                $(GTEST_DIR)/include/gtest/internal/*.h

GTEST_SRCS_ = $(GTEST_DIR)/src/*.cc $(GTEST_DIR)/src/*.h $(GTEST_HEADERS)

gtest-all.o : $(GTEST_SRCS_)
	$(CXX) $(CPPFLAGS) -I$(GTEST_DIR) $(CXXFLAGS) \
            -c $(GTEST_DIR)/src/gtest-all.cc

gtest_main.a : gtest-all.o
	$(AR) $(ARFLAGS) $@ $^


server: queryserver.c
	gcc $(CFLAGS) -g -I includes -o queryserver queryserver.c $(OBJS) -L.

multiserver: multiserver.c
	gcc $(CFLAGS) -g -I includes -o multiserver multiserver.c $(OBJS) -L. 

runserver:
	./queryserver data_small/ 1500

runmultiserver:
	./multiserver data_small/ 1500

client: queryclient.c 
	gcc $(CFLAGS) -g -I includes -o queryclient queryclient.c $(OBJS) -L.

runclient:
	./queryclient 127.0.0.1 1500

test: test_suite.o $(OBJS) queryclient.o gtest_main.a
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -lpthread $^ -o $@

test_suite.o: test_suite.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c test_suite.cc

queryprotocol.o: includes/queryprotocol.h src/queryprotocol.c
	$(CC) $(CFLAGS) -c src/queryprotocol.c
	mv queryprotocol.c libs/

clean: FORCE
	/bin/rm -f *.o *~ main
	rm multiserver
	rm queryserver
	rm queryclient
	rm test

FORCE:
