# Makefile for Glasgow ADT library
# Customized for laptop running CYGWIN_NT-6.2 on 15 Jan 2014 14:11:45
# 
OS=Linux
# Template makefile for Glasgow ADT library
#
# Conditionalized upon the value of $(OS) - if unspecified in the command
# line, OS is assumed to be the value defined above
#
# e.g.
#      make			# makes appropriate binaries for Windows_NT
#      make OS=Cygwin 		# makes appropriate binaries for Cygwin
#      make OS=Darwin 		# makes appropriate binaries for OSX
#      make OS=Linux 		# makes appropriate binaries for Linux

# base definitions
CC=gcc
# options to the compiler
OPTS=-O2
#
# you may need to append some additional flags to OPTS above; some examples
# we have encountered
#
# -D_GNU_SOURCE - if your pthread.h has conditionalized the definition of
#                 PTHREAD_MUTEX_RECURSIVE_NP and pthread_mutexattr_settype()
#                 seen on 32-bit RedHat Linux systems
#
# -Wno-strict-aliasing - if your compiler complains about breaking strict
#                        aliasing rules
#                        seen on 32-bit RedHat Linux systems
#
# if compiling for gdb or valgrind, replace "-O2" by "-g"
#

ifeq ($(OS),Cygwin)
    EXT=.exe
else
    EXT=
endif

ifeq ($(OS),Linux)
    LIBS=-lpthread
else
    LIBS=
endif

CFLAGS=-W -Wall -pedantic $(OPTS)
OBJECTS=iterator.o arraylist.o linkedlist.o hashmap.o bqueue.o uqueue.o treeset.o tsiterator.o tsarraylist.o tslinkedlist.o tshashmap.o tsbqueue.o tsuqueue.o tstreeset.o
PROGRAMS= altest$(EXT) hmtest$(EXT) lltest$(EXT) tstest$(EXT) tsaltest$(EXT) tslltest$(EXT) tshmtest$(EXT) tststest$(EXT) bqtest$(EXT) uqtest$(EXT)

all: $(PROGRAMS)

altest$(EXT): altest.c arraylist.h iterator.h libADTs.a $(LIBS)
	$(CC) -o altest$(EXT) $(CFLAGS) altest.c libADTs.a $(LIBS)

hmtest$(EXT): hmtest.c hashmap.h iterator.h libADTs.a $(LIBS)
	$(CC) -o hmtest$(EXT) $(CFLAGS) hmtest.c libADTs.a $(LIBS)

lltest$(EXT): lltest.c linkedlist.h iterator.h libADTs.a $(LIBS)
	$(CC) -o lltest$(EXT) $(CFLAGS) lltest.c libADTs.a $(LIBS)

tstest$(EXT): tstest.c treeset.h iterator.h libADTs.a $(LIBS)
	$(CC) -o tstest$(EXT) $(CFLAGS) tstest.c libADTs.a $(LIBS)

bqtest$(EXT): bqtest.c bqueue.h iterator.h libADTs.a $(LIBS)
	$(CC) -o bqtest$(EXT) $(CFLAGS) bqtest.c libADTs.a $(LIBS)

uqtest$(EXT): uqtest.c uqueue.h iterator.h libADTs.a $(LIBS)
	$(CC) -o uqtest$(EXT) $(CFLAGS) uqtest.c libADTs.a $(LIBS)

tsaltest$(EXT): tsaltest.c tsarraylist.h tsiterator.h libADTs.a $(LIBS)
	$(CC) -o tsaltest$(EXT) $(CFLAGS) tsaltest.c libADTs.a $(LIBS)

tslltest$(EXT): tslltest.c tslinkedlist.h tsiterator.h libADTs.a $(LIBS)
	$(CC) -o tslltest$(EXT) $(CFLAGS) tslltest.c libADTs.a $(LIBS)

tshmtest$(EXT): tshmtest.c tshashmap.h tsiterator.h libADTs.a $(LIBS)
	$(CC) -o tshmtest$(EXT) $(CFLAGS) tshmtest.c libADTs.a $(LIBS)

tststest$(EXT): tststest.c tstreeset.h tsiterator.h libADTs.a $(LIBS)
	$(CC) -o tststest$(EXT) $(CFLAGS) tststest.c libADTs.a $(LIBS)

libADTs.a: $(OBJECTS)
	rm -f libADTs.a
	ar r libADTs.a $(OBJECTS)
	ranlib libADTs.a

iterator.o: iterator.c iterator.h
arraylist.o: arraylist.c arraylist.h iterator.h
linkedlist.o: linkedlist.c linkedlist.h iterator.h
hashmap.o: hashmap.c hashmap.h iterator.h
treeset.o: treeset.c treeset.h iterator.h
bqueue.o: bqueue.c bqueue.h iterator.h
uqueue.o: uqueue.c uqueue.h iterator.h
tsiterator.o: tsiterator.c tsiterator.h
tsarraylist.o: tsarraylist.c tsarraylist.h tsiterator.h
tslinkedlist.o: tslinkedlist.c tslinkedlist.h tsiterator.h
tshashmap.o: tshashmap.c tshashmap.h tsiterator.h
tstreeset.o: tstreeset.c tstreeset.h tsiterator.h
tsuqueue.o: tsuqueue.c tsuqueue.h tsiterator.h
tsbqueue.o: tsbqueue.c tsbqueue.h tsiterator.h

clean:
	rm -f *.o *~ *.stackdump $(PROGRAMS) libADTs.a

