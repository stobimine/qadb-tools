
.SUFFIXES: .c .o .cc .c++

CPU     = freebsd
GCC     = gcc
CXX     = g++
LIBS    = -lstdc++ -lm
OBJECTS = parse.o qadb.o util.o heap.o irt.o
CFLAGS  = -ansi -I/usr/include -I/usr/local/include -g
#LDFLAGS = -L$(HOME)/$(CPU)/lib -L/usr/lib -L/usr/local/lib -lchasen -lstdc++
LDFLAGS = -L/usr/lib -L/usr/local/lib -lchasen -lstdc++

all: qadbman

install: qadbman
	cp align qadbman $(HOME)/$(CPU)/bin

chatest: parse.o chatest.o
	$(GCC) parse.o chatest.o $(LIBS) $(CDEFS) $(LDFLAGS) -o test

align: util.o align.o
	$(GCC) util.o align.o $(LIBS) $(CDEFS) $(LDFLAGS) -o align

qadbman: $(OBJECTS) qadbman.o
	$(GCC) $(OBJECTS) qadbman.o $(LIBS) $(CDEFS) $(LDFLAGS) -o qadbman

clean:
	rm -f *.o *~ a.out *.flc *.swp *.bak *.core test

distclean:
	rm -f chatest align qadbman

.cc.o: 
	$(CXX) $(CFLAGS) -c $<

.c.o:
	$(GCC) $(CFLAGS) -c $<

