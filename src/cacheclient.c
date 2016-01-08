/*
 * client for the Homework Database
 *
 * usage: ./cacheclient [-f stream] [-h host] [-p port] [-l packets] [-b nlines]
 *
 * each line from standard input (or a fifo) is assumed to be a complete
 * SQL query.  It is sent to the Homework database server, and the results
 * are printed on the standard output
 */

#include "config.h"
#include "util.h"
#include "rtab.h"
#include "srpc/srpc.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#define USAGE "./cacheclient [-f fifo] [-h host] [-p port] [-l packets] [-b nlines]"
#define MAX_LINE 4096
#define MAX_INSERTS 500

static char bf[MAX_LINE]; /* buffer to hold pushed-back line */
static int pback = 0; /* if bf holds a pushed-back line */

static int ifstream = 0;
static FILE *f;

static void pushback(char *line) {
    pback = 1;
    strcpy(bf, line);
}

static char *fetchline(char *line) {
    if (pback) {
        pback = 0;
        strcpy(line, bf);
        return line;
    }

    if (ifstream)
        return fgets(line, MAX_LINE, f);

    return fgets(line, MAX_LINE, stdin);
}

static void processresults(char *buf, int len, int log) {
    Rtab *results;
    char stsmsg[RTAB_MSG_MAX_LENGTH];

    results = rtab_unpack(buf, len);
    if (! results)
        printf("<< %s", buf);
    else if (rtab_status(buf, stsmsg)) /* error reported */
        printf("<< %s\n", stsmsg);
    else if (log)
        rtab_print(results);
    rtab_free(results);
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
    int ifbulk, ninserts;
    int ifsnapshot;
    char *inserts[MAX_INSERTS];
    int nreplies;
    char *service;

    host = HWDB_SERVER_ADDR;
    port = HWDB_SERVER_PORT;
    service = "HWDB";
    log = 1;
    ifbulk = 0;
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
        } else if (strcmp(argv[i], "-b") == 0) {
            ifbulk = 1;
            ninserts = atoi(argv[j]);
            if (ninserts > MAX_INSERTS)
                ninserts = MAX_INSERTS;
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
    if (!rpc_init(0)) {
        fprintf(stderr, "Failure to initialize rpc system\n");
        exit(-1);
    }
    if (!(rpc = rpc_connect(host, port, service, 1l))) {
        fprintf(stderr, "Failure to connect to %s at %s:%05u\n",
                service, host, port);
        exit(-1);
    }
    gettimeofday(&start, NULL);
    while (fetchline(inb) != NULL) {
        ifsnapshot = 0;
        if (strcmp(inb, "\n") == 0) /* ignore blank lines */
            continue;
        if (strcmp(inb, "BIGREDBUTTON\n") == 0) {
            sprintf(query, "SNAPSHOT:\n");
            n = strlen(query) + 1;	/* count '\0' */
            nreplies = 1;
            ifsnapshot++;
        } else if (ifbulk && strncmp(inb, "insert", 6) == 0) {
            int i, j, sofar;
            inserts[0] = strdup(inb);
            for (i = 1; i < ninserts; i++) {
                if (fetchline(inb) == NULL)
                    break;
                else if (strncmp(inb, "insert", 6) != 0) {
                    pushback(inb);
                    break;
                }
                inserts[i] = strdup(inb);
            }
            /* when we reach here, i is the number of inserts
             * that we have collected
             */
            sofar = 0;
            sofar += sprintf(query+sofar, "BULK:%d\n", i);
            for (j = 0; j < i; j++) {
                sofar += sprintf(query+sofar, "%s", inserts[j]);
                free(inserts[j]);
            }
            n = sofar + 1;
            nreplies = i;
        } else {
            sprintf(query, "SQL:%s", inb);
            n = strlen(query) + 1;	/* count '\0' */
            nreplies = 1;
        }
        if (log)
            printf(">> %s", query);
        count++;
        if (! rpc_call(rpc, Q_Arg(query), n, resp, sizeof(resp), &len)) {
            fprintf(stderr, "rpc_call() failed\n");
            break;
        }
        resp[len] = '\0';
        if (ifsnapshot)
            fprintf(stderr, "%s", resp);
        if (nreplies > 1) {	/* bulk insert */
            char *p, *q;
            int i, n;
            p = resp;
            for (i = 0; i < nreplies; i++) {
                q = p;
                p = strchr(q, '\n') + 1;
                n = p - q;
                strncpy(inb, q, n);
                inb[n] = '\0';
                processresults(inb, n, log);
            }
        } else {
            processresults(resp, len, log);
        }
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

