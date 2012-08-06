CC      = gcc
CFLAGS  = -Wall -O3
LDFLAGS = -ltcmalloc -lprofiler
INCLUDE = 

clmSOURCES = clm_main.c clm_utils.c clm_points.c clm_flags.c
clmOBJECTS = $(clmSOURCES:.c=.o)

all: clm

clm: $(clmOBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)  

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $< $(INCLUDE)

clean:
	rm -f *.o
cleanall:
	rm -f clm *.o
