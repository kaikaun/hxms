CC      = gcc
CFLAGS  = -std=gnu99 -Wall -O3
LDFLAGS = 

SOURCES = clustermatrix.c cluster.h

all: clustermatrix

clustermatrix: clustermatrix.o
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)  

.c.o:
	$(CC) $(CFLAGS) -c $< $(INCLUDE)

clean:
	rm -f *.o
cleanall:
	rm -f clustermatrix *.o
