/*
 * Copyright (c) 2013, Court of the University of Glasgow
 * All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:

 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * - Neither the name of the University of Glasgow nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Callback client over SRPC
 *
 * main thread
 *   creates a service named Handler (or name provided in command line)
 *   spins off handler thread
 *   connects to Callback
 *   sends connect request to Callback server
 *   sleeps for 5 minutes
 *   sends disconnect request to Callback server
 *   exits
 *
 * handler thread
 *   for each received event message
 *     prints the received event message
 *     sends back a response of "OK"
 */

#include "config.h"
#include "util.h"
#include "rtab.h"
#include "srpc.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>

#define USAGE "./registercallback [-h host] [-p port] [-s service] [-t minutes] -a automaton"

/*
 * global data shared by main thread and handler thread
 */
static RpcService rps;
static RpcConnection rpc;
static int auto_failure = 0;
static char id[25];	/* actually shared by main thread and signal handler */
static int sig_received = 0;
static int unregistered = 0;

static void unregister(void) {
    Q_Decl(query, 128);
    char resp[100];
    unsigned rlen;

    if (unregistered++)
        return;
    sprintf(query, "SQL:unregister %s", id);
    printf("Unregister query: %s\n", query);
    if (!rpc_call(rpc, Q_Arg(query), strlen(query)+1, resp, 100, &rlen)) {
        fprintf(stderr, "Error issuing unregister command\n");
        exit(1);
    }
    resp[rlen] = '\0';
    printf("Response to unregister command: %s", resp);
    /*
     * now disconnect from server
     */
    rpc_disconnect(rpc);
}

static void signal_handler(int signum) {
    sig_received = signum;
    unregister();
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
 * handler thread
 */
static void *handler(void *args) {
    char event[SOCK_RECV_BUF_LEN], resp[100], comment[256];
    RpcEndpoint sender;
    unsigned len, rlen;
    int status;

    while ((len = rpc_query(rps, &sender, event, SOCK_RECV_BUF_LEN)) > 0) {
        sprintf(resp, "OK");
        rlen = strlen(resp) + 1;
        rpc_response(rps, &sender, resp, rlen);
        event[len] = '\0';
        printf("%s", event);
        status = parse_response(event, comment);
        if (status) {
            fprintf(stderr, "Execution error for automaton: %s\n", comment);
            auto_failure++;
            break;
        }
    }
    return (args) ? NULL : args;	/* unused warning subterfuge */
}

#define DELAY 5		/* number of minutes to delay before unsubscribing */

static struct timespec time_delay = {0, 0};

/*
 *   creates a service named Handler
 *   spins off handler thread
 *   connects to Callback
 *   sends connect request to Callback server
 *   sleeps for 5 minutes
 *   sends disconnect request to Callback server
 *   exits
 */
int main(int argc, char *argv[]) {
    unsigned rlen;
    Q_Decl(query, 10000);
    char resp[100], myhost[100];
    unsigned short myport;
    pthread_t thr;
    int i, j;
    unsigned short port;
    char *target;
    char *service;
    char *automaton;
    int delay;
    int status;

    target = HWDB_SERVER_ADDR;
    port = HWDB_SERVER_PORT;
    service = "Handler";
    automaton = NULL;
    delay = DELAY;
    for (i = 1; i < argc; ) {
        if ((j = i + 1) == argc) {
            fprintf(stderr, "usage: %s\n", USAGE);
            exit(1);
        }
        if (strcmp(argv[i], "-h") == 0)
            target = argv[j];
        else if (strcmp(argv[i], "-p") == 0)
            port = atoi(argv[j]);
        else if (strcmp(argv[i], "-t") == 0)
            delay = atoi(argv[j]);
        else if (strcmp(argv[i], "-s") == 0)
            service = argv[j];
        else if (strcmp(argv[i], "-a") == 0)
            automaton = argv[j];
        else {
            fprintf(stderr, "Unknown flag: %s %s\n", argv[i], argv[j]);
        }
        i = j + 1;
    }
    if (automaton == NULL) {
        fprintf(stderr, "usage: %s\n", USAGE);
        exit(1);
    }

    time_delay.tv_sec = 60 * delay;
    /*
     * initialize the RPC system and offer the Callback service
     */
    if (!rpc_init(0)) {
        fprintf(stderr, "Initialization failure for rpc system\n");
        exit(-1);
    }
    rps = rpc_offer(service);
    if (! rps) {
        fprintf(stderr, "Failure offering %s service\n", service);
        exit(-1);
    }
    rpc_details(myhost, &myport);
    /*
     * start handler thread
     */
    if (pthread_create(&thr, NULL, handler, NULL)) {
        fprintf(stderr, "Failure to start timer thread\n");
        exit(-1);
    }
    /*
     * connect to HWDB service
     */
    rpc = rpc_connect(target, port, "HWDB", 1l);
    if (rpc == NULL) {
        fprintf(stderr, "Error connecting to HWDB at %s:%05u\n", target, port);
        exit(1);
    }
    /*
     * now register automaton
     */
    sprintf(query, "SQL:register \"%s\" %s %hu %s", automaton, myhost, myport, service);
    printf("%s\n", query);
    if (!rpc_call(rpc, Q_Arg(query), strlen(query)+1, resp, 100, &rlen)) {
        fprintf(stderr, "Error issuing register command\n");
        exit(1);
    }
    resp[rlen] = '\0';
    printf("Response to register command: %s", resp);
    status = parse_response(resp, id);
    if (status) {
        fprintf(stderr, "Error in registering automaton: %s\n", id);
        exit(1);
    }

    if (signal(SIGTERM, signal_handler) == SIG_IGN)
        signal(SIGTERM, SIG_IGN);
    if (signal(SIGINT , signal_handler) == SIG_IGN)
        signal(SIGINT , SIG_IGN);
    if (signal(SIGHUP , signal_handler) == SIG_IGN)
        signal(SIGHUP , SIG_IGN);

    /*
     * sleep for 5 minutes
     */
    nanosleep(&time_delay, NULL);
    /*
     * issue unsubscribe command
     */
    if (auto_failure) {
        fprintf(stderr, "Due to automaton failure, exiting ...\n");
        exit(1);
    }
    unregister();
    exit(0);
}
