CC=gcc
LEX=flex
CFLAGS=-W -Wall
#CFLAGS=-W -Wall -DDEBUG -g

ifeq ($(OS),Cygwin)
    EXT=.exe
    LIBS=
else
    EXT=
    LIBS=-pthread
endif

OBJECTS=agram.o automaton.o code.o topic.o scan.o gram.o event.o pubsub.o stack.o dsemem.o typetable.o sqlstmts.o nodecrawler.o mb.o rtab.o parser.o table.o indextable.o hwdb.o
PROGRAMS=cache$(EXT) cacheclient$(EXT) registercallback$(EXT) striplf$(EXT)
LIBS+=/usr/local/lib/libADTs.a /usr/local/lib/libsrpc.a -lm

all: $(PROGRAMS)

clean:
	rm -f *.o agram.c gram.h gram.c scan.c $(PROGRAMS) *~

cache$(EXT): cache.o hwdb.o rtab.o timestamp.o mb.o indextable.o topic.o automaton.o parser.o sqlstmts.o table.o typetable.o ptable.o nodecrawler.o event.o stack.o dsemem.o agram.o code.o gram.o scan.o
	gcc -o cache$(EXT) $^ $(LIBS)

cacheclient$(EXT): cacheclient.o rtab.o typetable.o sqlstmts.o timestamp.o
	gcc -o cacheclient$(EXT) $^ $(LIBS)

registercallback$(EXT): registercallback.o
	gcc -o registercallback$(EXT) $^ $(LIBS)

agram.c: agram.y code.h dataStackEntry.h machineContext.h timestamp.h event.h topic.h a_globals.h dsemem.h ptable.h stack.h automaton.h 
	yacc -o agram.c -p a_ agram.y

gram.h gram.c: gram.y util.h timestamp.h sqlstmts.h typetable.h config.h 
	yacc -o gram.c -d gram.y

scan.c: scan.l gram.h
	flex -t scan.l >scan.c

agram.o: agram.c
gram.o: gram.c
scan.o: scan.c

automaton.o: automaton.c automaton.h topic.h machineContext.h code.h stack.h timestamp.h dsemem.h a_globals.h event.h dataStackEntry.h
code.o: code.c dataStackEntry.h code.h event.h topic.h timestamp.h a_globals.h dsemem.h ptable.h typetable.h sqlstmts.h hwdb.h machineContext.h automaton.h rtab.h pubsub.h table.h stack.h config.h node.h
topic.o: topic.c topic.h event.h automaton.h dataStackEntry.h 
event.o: event.c event.h dataStackEntry.h topic.h timestamp.h code.h automaton.h machineContext.h stack.h
pubsub.o: pubsub.c pubsub.h 
stack.o: stack.c stack.h code.h dataStackEntry.h machineContext.h automaton.h event.h 
dsemem.o: dsemem.c dsemem.h dataStackEntry.h 
typetable.o: typetable.c typetable.h
sqlstmts.o: sqlstmts.c sqlstmts.h util.h timestamp.h gram.h typetable.h config.h
nodecrawler.o: nodecrawler.c nodecrawler.h util.h rtab.h sqlstmts.h table.h tuple.h timestamp.h gram.h mb.h node.h config.h typetable.h
mb.o: mb.c mb.h node.h table.h tuple.h timestamp.h sqlstmts.h rtab.h typetable.h config.h
rtab.o: rtab.c rtab.h util.h typetable.h sqlstmts.h config.h 
parser.o: parser.c parser.h sqlstmts.h typetable.h util.h timestamp.h gram.h config.h 
hwdb.o: hwdb.c hwdb.h mb.h util.h rtab.h sqlstmts.h parser.h indextable.h table.h pubsub.h automaton.h topic.h node.h tuple.h config.h typetable.h gram.h event.h timestamp.h dataStackEntry.h
table.o: table.c table.h util.h typetable.h sqlstmts.h pubsub.h node.h rtab.h config.h timestamp.h 
indextable.o: indextable.c indextable.h tuple.h table.h sqlstmts.h util.h nodecrawler.h typetable.h rtab.h pubsub.h topic.h ptable.h node.h config.h automaton.h dataStackEntry.h timestamp.h event.h
cache.o: cache.c config.h util.h hwdb.h rtab.h mb.h timestamp.h pubsub.h table.h automaton.h sqlstmts.h tuple.h node.h event.h typetable.h dataStackEntry.h 
timestamp.o: timestamp.c timestamp.h
ptable.o: ptable.c ptable.h dataStackEntry.h hwdb.h table.h nodecrawler.h topic.h tuple.h typetable.h rtab.h pubsub.h automaton.h sqlstmts.h node.h config.h event.h timestamp.h
cacheclient.o: cacheclient.c config.h util.h rtab.h 
registercallback.o: registercallback.c config.h util.h rtab.h 
