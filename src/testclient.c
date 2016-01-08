/*
 * testclient for the Homework Database
 *
 * usage: ./testclient [-f stream] [-h host] [-p port] [-l packets]
 *
 * each line from standard input (or a fifo) is assumed to be a complete
 * query.  It is sent to the Homework database server, and the results
 * are printed on the standard output
 */

#include "config.h"
#include "util.h"
#include <srpc/srpc.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "cacheconnect.h"

#define USAGE "./testclient [-f fifo] [-h host] [-p port] [-l packets]"
#define MAX_LINE 4096
#define MAX_INSERTS 500

static char bf[MAX_LINE]; /* buffer to hold pushed-back line */
static int pback = 0; /* if bf holds a pushed-back line */

static int ifstream = 0;
static FILE *f;

static char *fetchline(char *line) {
    char* ret;
    if (ifstream) {
        ret=fgets(line, MAX_LINE, f);
    } else {
        ret=fgets(line, MAX_LINE, stdin);
    }
    int t = strlen(ret);
    ret[t-1]='\0';
    return ret;
}

int dumpToStdout(CacheResponse r) {
    print_cache_response(r, stdout);
    return 0;
}

int main(int argc, char *argv[]) {
    RpcConnection rpc;
    char inb[MAX_LINE];
    Q_Decl(query, SOCK_RECV_BUF_LEN);
    char resp[SOCK_RECV_BUF_LEN];
    int n;
    unsigned len;
    char *host;
    unsigned short port;
    struct timeval start, stop;
    unsigned long count = 0;
    unsigned long msec;
    double mspercall;
    int i, j, log;
    char *service;
    CacheResponse cr;

    connect_env(&host, &port, &service);
    if(service == NULL) {
        service = "HWDB";
    }
    if(host == NULL) {
        host = HWDB_SERVER_ADDR;
    }
    if(port == 0) {
        port = HWDB_SERVER_PORT;
    }

    log = 1;
    for (i = 1; i < argc; ) {
        if ((j = i + 1) == argc) {
            fprintf(stderr, "usage: %s\n", USAGE);
            exit(1);
        }
        if (strcmp(argv[i], "-h") == 0)
            host = argv[j];
        else if (strcmp(argv[i], "-p") == 0)
            port = atoi(argv[j]);
        else if (strcmp(argv[i], "-s") == 0)
            service = argv[j];
        else if (strcmp(argv[i], "-f") == 0) {
            ifstream = 1;
            if ((f = fopen(argv[j], "r")) == NULL) {
                fprintf(stderr, "Failed to open %s\n", argv[j]);
                exit(-1);
            }
        } else if (strcmp(argv[i], "-l") == 0) {
            if (strcmp(argv[j], "packets") == 0)
                log++;
            else
                fprintf(stderr, "usage: %s\n", USAGE);
        } else {
            fprintf(stderr, "Unknown flag: %s %s\n", argv[i], argv[j]);
        }
        i = j + 1;
    }
    int err = init_cache(host, port, service);
    if(err) {
        fprintf(stderr, "connection failed\n");
        return 1;
    }
    gettimeofday(&start, NULL);
    while (fetchline(inb) != NULL) {
        if (strcmp(inb, "\n") == 0) /* ignore blank lines */
            continue;
        if(inb[0]=='S') {
            if(inb[1]=='F') {
                cr=file_sql(inb+2);
            } else {
                cr=raw_sql(inb+1);
            }
            if(cr) {
                print_cache_response(cr, stdout);
            } else {
                err=1;
            }
        } else if (inb[0]=='R') {
            if(inb[1]=='F') {
                err=install_file_automata(inb+2, dumpToStdout);
            } else {
                err=install_automata(inb+1, dumpToStdout);
            }
        } else {
            fprintf(stderr,"unrecognized action %c\n",inb[0]);
            continue;
        }
        if(err) {
            fprintf(stderr,"Could not process command: %s\n",inb);
            continue;
        }

        count++;
    }
    gettimeofday(&stop, NULL);
    if (stop.tv_usec < start.tv_usec) {
        stop.tv_usec += 1000000;
        stop.tv_sec--;
    }
    msec = 1000 * (stop.tv_sec - start.tv_sec) +
           (stop.tv_usec - start.tv_usec) / 1000;
    mspercall = 0;
    if (count > 0)
        mspercall = (double)msec / (double)count;
    fprintf(stderr, "%ld queries processed in %ld.%03ld seconds, %.3fms/call\n",
            count, msec/1000, msec%1000, mspercall);
    if (ifstream && f) fclose(f);
    rpc_disconnect(rpc);
    return 0;
}

