CC      = gcc
CFLAGS  = -std=gnu99 -Wall -D_THREAD_SAFE -D_REENTRANT
LIBDIR  = ../../11_lib
VPATH   = ../src:$(LIBDIR)
LDFLAGS = -L$(LIBDIR) -lmxml -lm -lpthread
INCLUDE = -I$(LIBDIR)

MXMLVER = mxml-2.7

dtSOURCES = preprocess.c mxmlmzXML.c easyzlib.c cdecode.c cencode.c
dtOBJECTS = $(dtSOURCES:.c=.o)

all: preprocess

preprocess: $(dtOBJECTS) libmxml.a
	$(CC) $(CFLAGS) -o ../$@ $(dtOBJECTS) $(LDFLAGS)

libmxml.a:
	rm -rf $(MXMLVER)/
	tar xzf $(LIBDIR)/$(MXMLVER).tar.gz
	cd $(MXMLVER);./configure
	make -C $(MXMLVER)/ libmxml.a
	mv $(MXMLVER)/libmxml.a $(LIBDIR)
	rm -rf $(MXMLVER)/

.c.o:
	$(CC) $(CFLAGS) -c $< $(INCLUDE)

clean:
	rm -f *.o
cleanall:
	rm -rf $(MXMLVER)/
	rm -f ../preprocess *.o $(LIBDIR)/libmxml.a
