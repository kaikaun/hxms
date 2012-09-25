CC      = gcc
CFLAGS  = -Wall -O3
LDFLAGS = 
VPATH   = ../
INCLUDE = -I ../

utSOURCES = unittest.c clm_utils.c clm_points.c clm_flags.c 
utOBJECTS = $(utSOURCES:.c=.o)

all: unittest readtest 

unittest: $(utOBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

readtest: readtest.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)  

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $< $(INCLUDE)

clean:
	rm -f *.o
cleanall:
	rm -f unittest readtest *.o
