CC      = gcc
CFLAGS  = -std=gnu99 -Wall -D_THREAD_SAFE -D_REENTRANT
LDFLAGS = -lm -lpthread -L./ -lmxml
VPATH   = ../src:../src/b64
INCLUDE =  -I ../src -I ../src/b64

MXMLVER = mxml-2.7

SOURCES = preprocess.c deadtime.c easyzlib.c mxmlmzXML.c cdecode.c cencode.c cluster.c

ppSOURCES = preprocess.c mxmlmzXML.c easyzlib.c cdecode.c cencode.c  
ppOBJECTS = $(ppSOURCES:.c=.o)

dtSOURCES = deadtime.c mxmlmzXML.c easyzlib.c cdecode.c cencode.c  
dtOBJECTS = $(dtSOURCES:.c=.o)

clSOURCES = cluster.c mxmlmzXML.c easyzlib.c cdecode.c cencode.c  
clOBJECTS = $(clSOURCES:.c=.o)

all: preprocess deadtime cluster

preprocess: $(ppOBJECTS) libmxml.a
	$(CC) $(CFLAGS) -o $@ $(ppOBJECTS) $(LDFLAGS)  
 
deadtime: $(dtOBJECTS) libmxml.a 
	$(CC) $(CFLAGS) -o $@ $(dtOBJECTS) $(LDFLAGS)

cluster: $(clOBJECTS) libmxml.a
	$(CC) $(CFLAGS) -o $@ $(clOBJECTS) $(LDFLAGS)  

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
