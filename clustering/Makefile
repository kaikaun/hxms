CC      = gcc
CFLAGS  = -std=gnu99 -Wall
LDFLAGS = 

SOURCES = clustermatrix.c cluster.c cluster.h

all: clustermatrix cluster

clustermatrix: clustermatrix.o
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)  
 
cluster: cluster.o
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

readtest: readtest.o
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

.c.o:
	$(CC) $(CFLAGS) -c $< $(INCLUDE)

clean:
	rm -f *.o
cleanall:
	rm -f clustermatrix cluster readtest *.o
