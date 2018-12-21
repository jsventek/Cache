##########################################################################################
AUTOMAKE_OPTIONS = foreign subdir-objects
ACLOCAL_FLAGS = -I m4

# library for clients
lib_LTLIBRARIES = libcache.la
include_HEADERS = cacheconnect.h
libcache_la_SOURCES = cacheconnect.c

# cache programs
bin_PROGRAMS = cache cacheclient registercallback lftocr testclient forwarder

cache_SOURCES = cache.c hwdb.c rtab.c timestamp.c mb.c indextable.c topic.c automaton.c parser.c sqlstmts.c table.c typetable.c ptable.c nodecrawler.c event.c stack.c dsemem.c agram.c code.c extreg.c gram.c scan.c gram.h agram.h scan.h parser.h

cacheclient_SOURCES = cacheclient.c rtab.c typetable.c sqlstmts.c timestamp.c

testclient_SOURCES = testclient.c 
testclient_LDADD = libcache.la

registercallback_SOURCES = registercallback.c

lftocr_SOURCES = lftocr.c

forwarder_SOURCES = forwarder.c rtab.c typetable.c sqlstmts.c timestamp.c

##########################################################################################
# Generated .c and .h
agram.c: agram.y code.h dataStackEntry.h machineContext.h timestamp.h event.h topic.h a_globals.h dsemem.h ptable.h stack.h automaton.h
	yacc -o agram.c -p a_ agram.y

gram.h gram.c: gram.y util.h timestamp.h sqlstmts.h typetable.h config.h logdefs.h 
	yacc -o gram.c -d gram.y

scan.c: scan.l gram.h
	flex -t scan.l >scan.c

parser.h: gram.h

hwdb.c: parser.h

mostlyclean-local:
	-rm -f agram.c gram.c scan.c
