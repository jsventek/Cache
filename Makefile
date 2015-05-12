CC=gcc
LEX=flex
CFLAGS=-W -Wall
#CFLAGS=-W -Wall -DDEBUG -g
OBJECTS=agram.o automaton.o code.o topic.o scan.o gram.o event.o pubsub.o stack.o dsemem.o typetable.o sqlstmts.o nodecrawler.o mb.o rtab.o parser.o table.o indextable.o hwdb.o
PROGRAMS=cache cacheclient registercallback striplf
LIBS=libADTs.a libsrpc.a
ALL_LIBS=$(LIBS) -pthread -lm

all: $(LIBS) $(PROGRAMS)

libADTs.a: adts/libADTs.a
	cp adts/libADTs.a .

adts/libADTs.a:
	(cd adts; make)

libsrpc.a: srpc/libsrpc.a
	cp srpc/libsrpc.a .

srpc/libsrpc.a:
	(cd srpc; make)

clean:
	(cd adts; make clean)
	(cd srpc; make clean)
	rm -f *.o scan.c agram.c gram.h gram.c $(LIBS) $(PROGRAMS) *~

striplf: striplf.o
	gcc -o striplf $^

cache: cache.o hwdb.o rtab.o timestamp.o mb.o indextable.o topic.o automaton.o parser.o sqlstmts.o table.o typetable.o ptable.o nodecrawler.o event.o stack.o dsemem.o agram.o code.o gram.o scan.o
	gcc -o cache $^ $(ALL_LIBS)

cacheclient: cacheclient.o rtab.o typetable.o sqlstmts.o timestamp.o
	gcc -o cacheclient $^ $(ALL_LIBS)

registercallback: registercallback.o
	gcc -o registercallback $^ $(ALL_LIBS)

agram.c: agram.y linkedlist.h hashmap.h code.h dataStackEntry.h machineContext.h timestamp.h event.h topic.h a_globals.h dsemem.h ptable.h iterator.h stack.h automaton.h tshashmap.h srpc.h tsiterator.h endpoint.h
	yacc -o agram.c -p a_ agram.y

gram.h gram.c: gram.y util.h timestamp.h sqlstmts.h linkedlist.h typetable.h config.h logdefs.h iterator.h
	yacc -o gram.c -d gram.y

scan.c: scan.l gram.h
	flex -t scan.l >scan.c

agram.o: agram.c
gram.o: gram.c
scan.o: scan.c

automaton.o: automaton.c automaton.h topic.h linkedlist.h tshashmap.h machineContext.h code.h stack.h timestamp.h dsemem.h a_globals.h srpc.h logdefs.h event.h iterator.h tsiterator.h hashmap.h dataStackEntry.h endpoint.h
code.o: code.c dataStackEntry.h hashmap.h code.h event.h topic.h timestamp.h a_globals.h dsemem.h ptable.h typetable.h sqlstmts.h hwdb.h linkedlist.h iterator.h machineContext.h automaton.h tshashmap.h rtab.h pubsub.h srpc.h table.h stack.h tsiterator.h config.h endpoint.h node.h
topic.o: topic.c topic.h event.h automaton.h tshashmap.h linkedlist.h dataStackEntry.h srpc.h tsiterator.h hashmap.h iterator.h endpoint.h
event.o: event.c event.h dataStackEntry.h topic.h timestamp.h code.h hashmap.h linkedlist.h automaton.h machineContext.h iterator.h srpc.h stack.h endpoint.h
pubsub.o: pubsub.c pubsub.h hashmap.h srpc.h iterator.h endpoint.h
stack.o: stack.c stack.h code.h dataStackEntry.h machineContext.h hashmap.h linkedlist.h automaton.h iterator.h event.h srpc.h endpoint.h
dsemem.o: dsemem.c dsemem.h dataStackEntry.h hashmap.h linkedlist.h iterator.h
typetable.o: typetable.c typetable.h
sqlstmts.o: sqlstmts.c sqlstmts.h util.h timestamp.h gram.h logdefs.h typetable.h config.h
nodecrawler.o: nodecrawler.c nodecrawler.h util.h linkedlist.h rtab.h sqlstmts.h table.h tuple.h timestamp.h gram.h mb.h node.h config.h logdefs.h iterator.h srpc.h typetable.h endpoint.h
mb.o: mb.c mb.h node.h table.h tuple.h timestamp.h linkedlist.h sqlstmts.h rtab.h srpc.h iterator.h typetable.h config.h endpoint.h
rtab.o: rtab.c rtab.h util.h typetable.h sqlstmts.h linkedlist.h config.h srpc.h logdefs.h iterator.h endpoint.h
parser.o: parser.c parser.h sqlstmts.h typetable.h util.h timestamp.h gram.h config.h logdefs.h
hwdb.o: hwdb.c hwdb.h mb.h util.h rtab.h sqlstmts.h parser.h indextable.h table.h hashmap.h pubsub.h srpc.h tslist.h automaton.h topic.h node.h tuple.h config.h logdefs.h typetable.h gram.h linkedlist.h iterator.h endpoint.h event.h timestamp.h dataStackEntry.h
table.o: table.c table.h util.h typetable.h sqlstmts.h pubsub.h srpc.h node.h linkedlist.h rtab.h config.h logdefs.h endpoint.h timestamp.h iterator.h
indextable.o: indextable.c indextable.h tuple.h hashmap.h table.h sqlstmts.h util.h nodecrawler.h typetable.h rtab.h srpc.h pubsub.h topic.h ptable.h linkedlist.h node.h iterator.h config.h logdefs.h endpoint.h automaton.h dataStackEntry.h timestamp.h event.h
cache.o: cache.c config.h util.h hwdb.h rtab.h srpc.h mb.h timestamp.h logdefs.h pubsub.h table.h automaton.h sqlstmts.h endpoint.h tuple.h node.h linkedlist.h event.h typetable.h iterator.h dataStackEntry.h hashmap.h
timestamp.o: timestamp.c timestamp.h
ptable.o: ptable.c ptable.h tshashmap.h dataStackEntry.h hwdb.h table.h nodecrawler.h topic.h tuple.h typetable.h tsiterator.h hashmap.h linkedlist.h rtab.h pubsub.h srpc.h automaton.h sqlstmts.h node.h iterator.h config.h endpoint.h event.h timestamp.h
cacheclient.o: cacheclient.c config.h util.h rtab.h srpc.h logdefs.h endpoint.h
registercallback.o: registercallback.c config.h util.h rtab.h srpc.h logdefs.h endpoint.h
