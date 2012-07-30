CC      = gcc
CFLAGS  = -Wall -O3
LDFLAGS = 
INCLUDE = 

clmSOURCES = clm_main.c clm_utils.c clm_points.c clm_flags.c
clmOBJECTS = $(clmSOURCES:.c=.o)

all: clustermatrix

clustermatrix: $(clmOBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)  

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $< $(INCLUDE)

clean:
	rm -f *.o
cleanall:
	rm -f clustermatrix *.o
