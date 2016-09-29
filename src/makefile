CC=g++
INC=-I./utils
CFLAGS=-c -Wall -std=c++0x -O2
LDFLAGS=-pthread
SOURCES=HSAlgo.cc main.cc
OBJECTS=$(SOURCES:.cc=.o)
EXECUTABLE=hs_leader

all:	$(SOURCES) $(EXECUTABLE)

$(EXECUTABLE):	$(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cc.o:
	$(CC) $(CFLAGS) $(INC) $< -o $@

clean:
	rm *.o hs_leader
