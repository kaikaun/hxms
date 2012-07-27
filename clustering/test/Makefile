CC      = gcc
CFLAGS  = -std=gnu99 -Wall -O3
LDFLAGS = 

SOURCES = unittest.c readtest.c ../cluster.h

all: unittest readtest 

unittest: unittest.o
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

readtest: readtest.o
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)  

.c.o:
	$(CC) $(CFLAGS) -c $< $(INCLUDE)

clean:
	rm -f *.o
cleanall:
	rm -f unittest readtest *.o
