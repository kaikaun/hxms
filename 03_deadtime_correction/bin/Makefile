CC      = gcc
CFLAGS  = -std=gnu99 -Wall -D_THREAD_SAFE -D_REENTRANT
LDFLAGS = -L./ -lmxml -lm -lpthread
VPATH   = ../src:../src/b64
INCLUDE =  -I ../src -I ../src/b64

MXMLVER = mxml-2.7

SOURCES = deadtime.c easyzlib.c mxmlmzXML.c cdecode.c cencode.c

dtSOURCES = deadtime.c mxmlmzXML.c easyzlib.c cdecode.c cencode.c  
dtOBJECTS = $(dtSOURCES:.c=.o)

all: deadtime

deadtime: $(dtOBJECTS) libmxml.a 
	$(CC) $(CFLAGS) -o $@ $(dtOBJECTS) $(LDFLAGS)

libmxml.a:
	rm -rf $(MXMLVER)/
	tar xzf $(MXMLVER).tar.gz
	cd $(MXMLVER);./configure
	make -C $(MXMLVER)/ libmxml.a
	mv $(MXMLVER)/libmxml.a .
	rm -rf $(MXMLVER)/

.c.o:
	$(CC) $(CFLAGS) -c $< $(INCLUDE)

clean:
	rm -f *.o
cleanall:
	rm -rf $(MXMLVER)
	rm -f preprocess deadtime *.o *.a
