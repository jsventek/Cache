/*
 * Cache Forwarding Agent
 *
 * receives forwarded tuples and join commands from other forwarders
 * registers automaton with local Cache instance to receive tuples inserted
 *           into that Cache for particular tables
 * automaton thread forwards those tuples to the other forwarders
 */

#include "config.h"
#include "util.h"
#include "hwdb.h"
#include "rtab.h"
#include "srpc/srpc.h"
#include "mb.h"
#include "timestamp.h"
#include "adts/hashmap.h"
#include "adts/tshashmap.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pthread.h>

#define USAGE "./forwarder [-p f_port] [-c c_port] [-l none|overlay|forward|messages|all] -f f_config -t t_config"
#define LOG_NONE 0
#define LOG_OVERLAY 1
#define LOG_FORWARD 2
#define LOG_MESSAGES 3
#define LOG_ALL 4
#define ILLEGAL_QUERY_RESPONSE "1<|>Illegal query<|>0<|>0<|>\n"
#define ACCEPTED_FORWARD_RESPONSE "0<|>Tuple accepted<|>0<|>0<|>\n"

#define log_it(level, ...) {if (log_level >= level) {fprintf(stderr, "%d ", level); fprintf(stderr, __VA_ARGS__);}}

typedef struct forwarder {
    char *hostname;
    int port;
} Forwarder;

typedef struct ft_entry {
    int port;
    RpcConnection rpc;
} FTEntry;

typedef struct ts_table {
    pthread_mutex_t lock;
    HashMap *table;
} TSTable;

/*
 * global data shared by main thread and handler thread
 */
static char *progname = NULL;
static unsigned short c_port = 0, f_port = 0;
static int log_level = 0;
static char *f_config = NULL, *t_config = NULL;

static RpcService rpsfto = NULL;	/* forward service other forwarders */
static RpcService rpsftc = NULL;	/* forward service to Cache */
static RpcConnection rpc = NULL;	/* connection to Cache */
static int must_exit = 0;
static int sig_received = 0;
static int auto_failure = 0;
static int unregistered = 0;
static char id[25];			/* ident of automaton */
static TSTable timestamps;		/* table of inserted timestamps */
static TSHashMap *forw_table = NULL;	/* forwarding table */
static char my_host[256];		/* fully qualified hostname */

static char buf[SOCK_RECV_BUF_LEN];
static char resp[SOCK_RECV_BUF_LEN];

static void unregister(void) {
    Q_Decl(query, 128);
    char resp[100];
    unsigned rlen;

    if (unregistered++)
        return;
    sprintf(query, "SQL:unregister %s", id);
    log_it(LOG_NONE, "Unregister query: %s\n", query);
    if (!rpc_call(rpc, Q_Arg(query), strlen(query)+1, resp, 100, &rlen)) {
        fprintf(stderr, "Error issuing unregister command\n");
        exit(-1);
    }
    resp[rlen] = '\0';
    log_it(LOG_NONE, "Response to unregister command: %s", resp);
    /*
     * now disconnect from Cache
     */
    rpc_disconnect(rpc);
}

static void signal_handler(int signum) {
    sig_received = signum;
    unregister();
    must_exit++;
    rpc_shutdown();
    fflush(stdout);
    exit(1);
}

static int parse_response(char *resp, char *comment) {
    char *p, *q, *r;
    int n = 0;

    q = strstr(resp, "<|>");
    for (p = resp; p != q; p++) {
        n = 10 * n + (*p - '0');
    }
    p = q + 3;
    q = strstr(p, "<|>");
    r = comment;
    while (p != q) {
        *r++ = *p++;
    }
    *r = '\0';
    return n;
}

/*
 * return pointer just beyond the n'th occurrence of a substring
 *
 * returns NULL if 'str' does not contain at least 'n' instances of 'sub'
 */
static char *skip_n(char *str, char *sub, int n) {
    char *p, *q;
    int sublen = strlen(sub);
    int i;

    for (p = str, i = 0; (i < n) && ((q = strstr(p, sub)) != NULL); p = q + sublen, i++)
        ;
    /* we arrive here if i == n || strstr returned NULL */
    return (q == NULL) ? NULL : p;
}

/*
 * fetch characters up to, but not including substring
 *
 * fetch terminates when substring is found OR '\0' is reached
 */
static char *fetch_next(char *str, char *sub, char *buf) {
    char *p = strstr(str, sub);

    if (p == NULL) {
        strcpy(buf, str);
        return NULL;
    } else {
        while (str < p) {
            *buf++ = *str++;
        }
        *buf = '\0';
        p += strlen(sub);
        return p;
    }
}

/*
 * handler thread function
 */
static void *handler(void *args) {
    char event[SOCK_RECV_BUF_LEN], resp[100], comment[256];
    char topic[100];
    Q_Decl(insert, 256);
    RpcEndpoint sender;
    unsigned len, rlen;
    int status;
    void *dummy;
    TSIterator *it;
    char *p, *q;
    int i;
    int removed;

    while ((len = rpc_query(rpsfto, &sender, event, SOCK_RECV_BUF_LEN)) > 0) {
        sprintf(resp, "OK");
        rlen = strlen(resp) + 1;
        rpc_response(rpsfto, &sender, resp, rlen);
        event[len] = '\0';
        log_it(LOG_MESSAGES, "%s", event);
        status = parse_response(event, comment);
        if (status) {
            fprintf(stderr, "Execution error for automaton: %s\n", comment);
            auto_failure++;
            break;
        }
        p = skip_n(event, "\n", 2);	/* skip over status and column lines */
        p = skip_n(p, "<|>",1);		/* skip over extra timestamp */
        p = fetch_next(p, "<|>", topic);/* fetch topic */
        p = fetch_next(p, "<|>", comment);	/* fetch timestamp */
        pthread_mutex_lock(&(timestamps.lock));
        removed = hm_remove(timestamps.table, comment, (void **)&dummy);
        pthread_mutex_unlock(&(timestamps.lock));
        if (removed) {
            log_it(LOG_FORWARD, "Remove tuple(%s) from timestamp table\n", comment);
            continue;
        }
        q = insert;
        q += sprintf(q, "SQL:insert into %s values (", topic);
        for (i = 0; (p = fetch_next(p, "<|>", comment)) != NULL; i++)
            q += sprintf(q, "%s'%s'", (i == 0)?"":", ", comment);
        sprintf(q, ")");
        it = tshm_it_create(forw_table);
        while (tsit_hasNext(it)) {
            HMEntry *he;
            (void)tsit_next(it, (void **)&he);
            if (strcmp(my_host, hmentry_key(he)) != 0) {
                log_it(LOG_FORWARD, "%s -> %s\n", hmentry_key(he), insert);
            } else {
                log_it(LOG_FORWARD, "%s -> %s\n", hmentry_key(he), insert);
            }
        }
        tsit_destroy(it);
        pthread_mutex_unlock(&(timestamps.lock));
    }
    return (args) ? NULL : args;	/* unused warning subterfuge */
}

static LinkedList *read_forwarders(char *file, int log) {
    FILE *fd;
    LinkedList *ll;
    Forwarder *f;
    char *p;
    char buf[100];

    if ((fd = fopen(file, "r")) == NULL) {
        fprintf(stderr, "Unable to open Forwarder file %s\n", file);
        return NULL;
    }
    if ((ll = ll_create()) == NULL) {
        fclose(fd);
        fprintf(stderr, "Unable to create linked list of forwarders\n");
        return NULL;
    }
    while (fgets(buf, sizeof(buf), fd) != NULL) {
        int len = strlen(buf) - 1;
        buf[len] = '\0';	/* replace \n by \0 */
        log_it(LOG_OVERLAY, "read_forwarders: %s\n", buf);
        p = strchr(buf, ':');
        if (p == NULL) {
            fprintf(stderr, "Incorrectly formatted entry: %s\nMust be host:port\n", buf);
            continue;
        }
        *p++ = '\0';
        f = (Forwarder *)malloc(sizeof(Forwarder));
        f->hostname = strdup(buf);
        f->port = atoi(p);
        ll_addLast(ll, (void *)f);
    }
    fclose(fd);
    return ll;
}

/*
 * Reads table names from file, one per line
 *
 * returns a linked list of the table names
 */
static LinkedList *read_tables(char *file, int log) {
    FILE *fd;
    LinkedList *ll;
    char *t;
    char buf[100];

    if ((fd = fopen(file, "r")) == NULL) {
        fprintf(stderr, "Unable to open Table file %s\n", file);
        return NULL;
    }
    if ((ll = ll_create()) == NULL) {
        fclose(fd);
        fprintf(stderr, "Unable to create linked list of table names\n");
        return NULL;
    }
    while (fgets(buf, sizeof(buf), fd) != NULL) {
        int len = strlen(buf) - 1;
        buf[len] = '\0';	/* replace \n by \0 */
        log_it(LOG_OVERLAY, "read_tables: %s\n", buf);
        t = strdup(buf);
        ll_addLast(ll, (void *)t);
    }
    fclose(fd);
    return ll;
}

static void print_cmd(int level, char *str) {
    char *p;

    fprintf(stderr, "%d ", level);
    for (p = str; *p != '\0'; p++)
        if (*p == '\r')
            fprintf(stderr, "\n  ");
        else
            fputc(*p, stderr);
    fputc('\n', stderr);
}

static void dump_forw_table(int level) {
    TSIterator *iter;
    HMEntry *he;

    iter = tshm_it_create(forw_table);
    log_it(level, "Current contents of forwarding table:\n");
    while (tsit_hasNext(iter)) {
        char *key;
        FTEntry *fte;
        (void) tsit_next(iter, (void **)&he);
        key = hmentry_key(he);
        fte = (FTEntry *)hmentry_value(he);
        log_it(level, "%s:%05u\n", key, (unsigned short)fte->port);
    }
    tsit_destroy(iter);
}

static void process_args(int argc, char *argv[]) {
    int i, j;

    progname = argv[0];
    c_port = HWDB_SERVER_PORT;
    f_port = CACHE_FORWARDER_PORT;
    log_level = LOG_NONE;
    f_config = NULL;
    t_config = NULL;
    for (i = 1; i < argc; ) {
        if ((j = i + 1) == argc) {
            fprintf(stderr, "usage: %s\n", USAGE);
            exit(-1);
        }
        if (strcmp(argv[i], "-p") == 0) {
            f_port = (unsigned short) atoi(argv[j]);
        } else if (strcmp(argv[i], "-c") == 0) {
            c_port = (unsigned short) atoi(argv[j]);
        } else if (strcmp(argv[i], "-l") == 0) {
            if (strcmp(argv[j], "none") == 0)
                log_level = LOG_NONE;
            else if (strcmp(argv[j], "overlay") == 0)
                log_level = LOG_OVERLAY;
            else if (strcmp(argv[j], "forward") == 0)
                log_level = LOG_FORWARD;
            else if (strcmp(argv[j], "messages") == 0)
                log_level = LOG_MESSAGES;
            else if (strcmp(argv[j], "all") == 0)
                log_level = LOG_ALL;
            else {
                fprintf(stderr, "usage: %s\n", USAGE);
            }
        } else if (strcmp(argv[i], "-f") == 0) {
            f_config = argv[j];
        } else if (strcmp(argv[i], "-t") == 0) {
            t_config = argv[j];
        } else {
            fprintf(stderr, "Unknown flag: %s %s\n", argv[i], argv[j]);
        }
        i = j + 1;
    }
    if (f_config == NULL || t_config == NULL) {
        fprintf(stderr, "usage: %s\n", USAGE);
        exit(-1);
    }
}

int main(int argc, char *argv[]) {
    unsigned short my_port;
    char my_ip[16];
    char automaton[10000];
    unsigned rlen;
    Q_Decl(query, 10000);
    int i, j;
    LinkedList *target, *tables;
    pthread_t thr;
    FTEntry *fte;
    void *dummy;
    int status;
    char *p, *t;
 
    process_args(argc, argv);
    log_it(LOG_NONE, "obtaining target list of forwarders\n");
    if (!(target = read_forwarders(f_config, log_level))) {
        fprintf(stderr, "Unable to read forwarding config file %s\n", f_config);
        exit(-1);
    }
    log_it(LOG_NONE, "obtaining tables which should be forwarded\n");
    if (!(tables = read_tables(t_config, log_level))) {
        fprintf(stderr, "Unable to read tables config file %s\n", t_config);
        exit(-1);
    }
    log_it(LOG_NONE, "initializing rpc system\n");
    if (!rpc_init(f_port)) {
        fprintf(stderr, "Failure to initialize rpc system\n");
        exit(-1);
    }
    log_it(LOG_NONE, "offering ForwarderToOthers service\n");
    rpsfto = rpc_offer("ForwarderToOthers");
    if (! rpsfto) {
        fprintf(stderr, "Failure offering ForwarderToOthers service\n");
        exit(-1);
    }
    log_it(LOG_NONE, "offering ForwarderToCache service\n");
    rpsftc = rpc_offer("ForwarderToCache");
    if (! rpsftc) {
        fprintf(stderr, "Failure offering ForwarderToCache service\n");
        exit(-1);
    }
    rpc_details(my_ip, &my_port);
    rpc_reverselu(my_ip, my_host);
    log_it(LOG_NONE, "forwarding service at %s:%05u\n", my_host, my_port);
    /*
     * start forwarding thread
     */
    if (pthread_create(&thr, NULL, handler, NULL)) {
        fprintf(stderr, "Failure to start forwarding thread\n");
        exit(-1);
    }
    /*
     * connect to Cache service
     */
    rpc = rpc_connect("localhost", c_port, "HWDB", 1l);
    if (rpc == NULL) {
        fprintf(stderr, "Error connecting to Cache at localhost:%05u\n", c_port);
    exit(-1);
    }

    /*
     * initialize timestamp and forwarding tables
     */
    timestamps.table = hm_create(100, 0.5);	/* hashmap to hold timestamps */
    pthread_mutex_init(&(timestamps.lock), NULL);
    forw_table = tshm_create(20, 0.5);	/* thread-safe hashmap for forw table */
    fte = (FTEntry *)malloc(sizeof(FTEntry));
    fte->port = (int)my_port;
    fte->rpc = NULL;
    tshm_put(forw_table, my_host, (void *)fte, &dummy);
    dump_forw_table(log_level);

    /*
     * now register automaton
     */
    /* build automaton from tables */
    p = automaton;
    for (i = 1; ll_removeFirst(tables, (void **)&t); i++) {
        p += sprintf(p, "subscribe t%d to %s;\r", i, t);
        free(t);
    }
    sprintf(p, "behavior {\r    send(currentTopic(), currentEvent());\r}\r");
    /* build query */
    sprintf(query, "SQL:register \"%s\" %s %hu ForwarderToOthers",
            automaton, my_ip, my_port);
    /* log command */
    if (log_level >= LOG_FORWARD)
        print_cmd(LOG_FORWARD, query);
    if (! rpc_call(rpc, Q_Arg(query), strlen(query)+1, resp, 100, &rlen)) {
        fprintf(stderr, "Error issuing register command\n");
        exit(-1);
    }
    resp[rlen] = '\0';
    log_it(LOG_FORWARD, "Response to register command: %s", resp);
    status = parse_response(resp, id);
    if (status) {
        fprintf(stderr, "Error in registering automaton: %s\n", id);
        exit(-1);
    }
    /*
     * establish signal handlers
     */
    if (signal(SIGTERM, signal_handler) == SIG_IGN)
        signal(SIGTERM, SIG_IGN);
    if (signal(SIGINT, signal_handler) == SIG_IGN)
        signal(SIGINT, SIG_IGN);
    if (signal(SIGHUP , signal_handler) == SIG_IGN)
        signal(SIGHUP , SIG_IGN);

    /* legal queries are of the following form:
     *
     * SQL:<legal insert statement>\n
     *
     * JOIN: <hostname>:<port>\n
     *
     * For SQL insert queries, the response will consist of a line of the form
     *
     * status<|>Status comment<|>0<|>0<|>\n
     *
     * For JOIN commands, the response will consist of a line
     *
     * status<|>Status comment<|>ncols<|>nrows<|>\n
     *
     * if nrows > 0, subsequent lines in the response will consist of
     *     column descriptors (varchar:hostname, int:port),
     *     followed by column values for each element of the forwarding table
     */
    while (! must_exit) {
        int len;
        RpcEndpoint sender;
        char *p;

        if ((len = rpc_query(rpsftc, &sender, buf, SOCK_RECV_BUF_LEN)) == 0)
            break;
        buf[len] = '\0';
        log_it(LOG_MESSAGES, "Received: %s", buf);
        p = strchr(buf, ':');
        if (p == NULL) {
            fprintf(stderr, "Illegal query: %s\n", buf);
            strcpy(resp, ILLEGAL_QUERY_RESPONSE);
            len = strlen(resp) + 1;
            rpc_response(rpsftc, &sender, resp, len);
            continue;
        }
        *p = '\0';
        if (strcmp(buf, "SQL") == 0) {
            char *q;
            char timestamp[20];
            /* first, indicate we have accepted the forwarded tuple */
            sprintf(resp, ACCEPTED_FORWARD_RESPONSE);
            len = strlen(resp) + 1;
            rpc_response(rpsftc, &sender, resp, len);
            /* now prepare to call the Cache to insert the tuple */
	    *p = ':'; /* replace the colon */
            p = strchr(buf, '\n');
            if (p)
                *p++ ='\0';
            /* call cache with insert command */
            strcpy(query, buf);
            log_it(LOG_MESSAGES, "Sent to Cache: %s\n", query);
            pthread_mutex_lock(&(timestamps.lock));
            if (! rpc_call(rpc, Q_Arg(query), strlen(query)+1, resp, 100, &rlen)) {
                fprintf(stderr, "Error issuing insert command to Cache\n");
                continue;
            }
            log_it(LOG_ALL, "Call to Cache completed\n");
            status = parse_response(resp, timestamp);
            if (status) {
                fprintf(stderr, "Error inserting tuple into Cache: %s\n", timestamp);
            } else {
                void *dummy;
                (void) hm_put(timestamps.table, timestamp, NULL, (void **)&dummy);
                log_it(LOG_FORWARD, "Insert tuple(%s) into timestamp table\n", timestamp);
            }
            pthread_mutex_unlock(&(timestamps.lock));
        } else if (strcmp(buf, "JOIN") == 0) {
            char *q;
            q = ++p; /* move past the colon position */
            p = strchr(q, '\n');
            if (p)
                *p++ = '\0';
            /* process join request */
        } else {
            fprintf(stderr, "Illegal query: %s:%s\n", buf, p);
            strcpy(resp, ILLEGAL_QUERY_RESPONSE);
            len = strlen(resp) + 1;
        }
        rpc_response(rpsftc, &sender, resp, len);
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
