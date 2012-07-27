CC      = gcc
CFLAGS  = -std=gnu99 -Wall -O3
LDFLAGS = 

SOURCES = cluster.c

all: cluster

cluster: cluster.o
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

.c.o:
	$(CC) $(CFLAGS) -c $< $(INCLUDE)

clean:
	rm -f *.o
cleanall:
	rm -f cluster *.o
