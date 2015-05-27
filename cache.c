/*
 * Homework Cache
 *
 * single-threaded provider of the Homework Database using SRPC
 *
 * expects SQL statement in input buffer, sends back results of
 * query in output buffer
 */

#include "config.h"
#include "util.h"
#include "hwdb.h"
#include "rtab.h"
#include "srpc.h"
#include "mb.h"
#include "timestamp.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define USAGE "./cache [-p port] [-l packets|stats] [-c config-file]"
#define LOG_STATS 1
#define LOG_PACKETS 2
#define STATS_COUNT 10000
#define ILLEGAL_QUERY_RESPONSE "1<|>Illegal query<|>0<|>0<|>\n"

char *progname;
int must_exit = 0;
int sig_received = 0;
extern int log_allocation;

static char buf[SOCK_RECV_BUF_LEN];
static char resp[SOCK_RECV_BUF_LEN];

static void signal_handler(int signum) {
    sig_received = signum;
    must_exit++;
    rpc_shutdown();
}

static void loadfile(char *file, int log, int isreadonly) {
    FILE *fd;
    int len;
    Rtab *results;
    char stsmsg[RTAB_MSG_MAX_LENGTH];

    if (!(fd = fopen(file, "r"))) {
        fprintf(stderr, "Unable to open configuration file %s\n", file);
        return;
    }
    while (fgets(buf, sizeof(buf), fd) != NULL) {
        len = strlen(buf) - 1;	/* get rid of \n */
        if (len == 0)
            continue;		/* ignore empty lines */
        if (buf[0] == '#')
            continue; /* ignore comments */
        buf[len] = '\0';
        if (log)
            printf(">> %s\n", buf);
        results = hwdb_exec_query(buf, isreadonly);
        if (! results)
            strcpy(resp, ILLEGAL_QUERY_RESPONSE);
        else
            (void) rtab_pack(results, resp, SOCK_RECV_BUF_LEN, &len);
        if (rtab_status(resp, stsmsg))
            printf("<< %s\n", stsmsg);
        else if (log)
            rtab_print(results);
        rtab_free(results);
    }
    fclose(fd);
}

int main(int argc, char *argv[]) {
    RpcEndpoint sender;
    unsigned len;
    RpcService rps;
    unsigned short port, snap;
    int i, j;
    Rtab *results;
    int log, count;
    char *p, *q, *r;
    int ninserts, sofar;
    char *cfile;
    tstamp_t start, finish;
    int isreadonly;

    port = HWDB_SERVER_PORT;
    snap = HWDB_SNAPSHOT_PORT;
    log = LOG_STATS;
    cfile = NULL;
    isreadonly = 0;
    for (i = 1; i < argc; ) {
        if ((j = i + 1) == argc) {
            fprintf(stderr, "usage: %s\n", USAGE);
            exit(1);
        }
        if (strcmp(argv[i], "-p") == 0) {
            port = atoi(argv[j]);
            snap = port + 1;
        } else if (strcmp(argv[i], "-l") == 0) {
            if (strcmp(argv[j], "packets") == 0)
                log = LOG_PACKETS;
            else if (strcmp(argv[j], "stats") == 0)
                log = LOG_STATS;
            else {
                fprintf(stderr, "usage: %s\n", USAGE);
            }
        } else if (strcmp(argv[i], "-c") == 0) {
            cfile = argv[j];
        } else {
            fprintf(stderr, "Unknown flag: %s %s\n", argv[i], argv[j]);
        }
        i = j + 1;
    }
    printf("initializing database\n");
    hwdb_init(1);
    if (cfile) {
        printf("processing configuration file %s\n", cfile);
        loadfile(cfile, log, isreadonly);
    }
    printf("initializing rpc system\n");
    if (!rpc_init(port)) {
        fprintf(stderr, "Failure to initialize rpc system\n");
        exit(-1);
    }
    printf("offering service\n");
    rps = rpc_offer("HWDB");
    if (! rps) {
        fprintf(stderr, "Failure offering HWDB service\n");
        exit(-1);
    }
    printf("starting to read queries from network\n");
    //log_allocation = 1;
    count = 0;

    if (signal(SIGTERM, signal_handler) == SIG_IGN)
        signal(SIGTERM, SIG_IGN);
/*    if (signal(SIGINT , signal_handler) == SIG_IGN)
        signal(SIGINT , SIG_IGN);
*/    if (signal(SIGHUP , signal_handler) == SIG_IGN)
        signal(SIGHUP , SIG_IGN);

    /* legal queries are of the following form:
     *
     * SQL:<legal sql statement>\n
     *
     * BULK:<number>\n
     * insert into .....\n  --+
     * insert into .....\n    |
     * ...                     > <number> of these
     * ...                    |
     * insert into .....\n  --+
     *
     * SNAPSHOT:\n
     *
     * For SQL queries, the response will consist of a line of the form
     *
     * status<|>Status comment<|>ncols<|>nrows<|>\n
     *
     * if nrows > 0, subsequent lines in the response will consist of
     * column descriptors, followed by column values for each row
     *
     * for BULK inserts, the response will consist of <number> lines, each
     * of the form
     *
     * status<|>Status comment<|>0<|>0<|>\n
     *
     * For SNAPSHOT commands, the response will consist of a line
     *
     * status<|>Status comment<|>0<|>0<|>\n
     */
    while (! must_exit) {
        if ((len = rpc_query(rps, &sender, buf, SOCK_RECV_BUF_LEN)) == 0)
            break;
        buf[len] = '\0';
        if (log >= LOG_PACKETS) {
            MSG("Received: %s", buf);
        }
        p = strchr(buf, ':');
        if (p == NULL) {
            printf("Illegal query: %s\n", buf);
            strcpy(resp, ILLEGAL_QUERY_RESPONSE);
            len = strlen(resp) + 1;
            rpc_response(rps, &sender, resp, len);
            continue;
        }
        *p++ = '\0';
        if (strcmp(buf, "SQL") == 0) {
            count++;
            q = p;
            p = strchr(q, '\n');
            if (p)
                *p++ ='\0';
            results = hwdb_exec_query(q, isreadonly);
            if (log >= LOG_PACKETS) {
                rtab_print(results);
            }
            if (! results) {
                strcpy(resp, "1<|>Error<|>0<|>0<|>\n");
                len = strlen(resp) + 1;
            } else {
                if (! rtab_pack(results, resp, SOCK_RECV_BUF_LEN, &i))
                    printf("query results truncated\n");
                len = i;
            }
            rtab_free(results);
        } else if (strcmp(buf, "BULK") == 0) {
            q = p;
            p = strchr(q, '\n');
            *p++ = '\0';
            ninserts = atoi(q);
            r = resp;
            sofar = 0;
            for (j = 0; j < ninserts; j++) {
                q = p;
                p = strchr(q, '\n');
                *p++ = '\0';
                count++;
                results = hwdb_exec_query(q, isreadonly);
                if (log >= LOG_PACKETS) {
                    rtab_print(results);
                }
                if (! results) {
                    sofar += sprintf(r+sofar, "1<|>Error<|>0<|>0<|>\n");
                } else {
                    (void) rtab_pack(results, r+sofar, SOCK_RECV_BUF_LEN, &i);
                    sofar += i;
                }
                rtab_free(results);
            }
            len = sofar;
        } else if (strcmp(buf, "SNAPSHOT") == 0) {
            start = timestamp_now();
            rpc_suspend();		/* suspend RPC processing */
            pid_t pid = fork();
            if (pid != 0) {
                if (pid == -1) {
                    rpc_resume();
                    sprintf(resp, "1<|>Snapshot fork err<|>0<|>0<|>\n");
                    len = strlen(resp) + 1;
                } else if (pid != 0) {	/* parent branch */
                    int status;
                    (void)wait(&status);
                    rpc_resume();
                    finish = (timestamp_now() - start) / 100000;
                    if (! status)
                        sprintf(resp, "0<|>Snapshot success, port=%hu, %lld.%lld ms<|>0<|>0<|>\n", snap, finish/10, finish%10);
                    else
                        sprintf(resp, "1<|>Snapshot fork err<|>0<|>0<|>\n");
                    len = strlen(resp) + 1;
                }
            } else {			/* child branch */
                pid_t pid = fork();	/* zombie-free zone */
                if (pid == -1)
                    exit(1);
                else if (pid != 0)
                    exit(0);
                if (! rpc_reinit(snap))
                    exit(1);
                setsid();		/* new session */
                isreadonly = 1;
                continue;		/* no response to send */
            }
        } else {
            printf("Illegal query: %s:%s\n", buf, p);
            strcpy(resp, ILLEGAL_QUERY_RESPONSE);
            len = strlen(resp) + 1;
        }
        rpc_response(rps, &sender, resp, len);
        if (count >= STATS_COUNT) {
            count = 0;
            if (log >= LOG_STATS)
                mb_dump();
        }
    }
    /*
     * we reach here if a signal is received or rpc_query yields 0
     */
    if (must_exit) {
        fprintf(stderr, "signal %d received\n", sig_received);
    } else {
        fprintf(stderr, "rpc_query failure\n");
    }
    return 0;
}
