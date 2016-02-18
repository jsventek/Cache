/*
 * Copyright (c) 2016, University of Oregon
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <srpc/srpc.h>
#include <adts/hashmap.h>

#include "cacheconnect.h"

#define BUFLEN 1024

/* Response Data */
struct cache_response_t {
    int retcode;
    char* message;
    int ncols;
    int nrows;

    char** headers;
    char** data;
};

/// Returns the pointer to the next char in p to scan from
static const char *separator = "<|>"; /* separator between packed fields */
static char* fetch_str(char *p, char **str) {
    char *q, c;
    if(*p=='\n') { p++; }
    if ((q = strstr(p, separator)) != NULL) {
        c = *q;
        *q = '\0';
        *str = strdup(p);
        *q = c;
        q += strlen(separator);
    } else
        *str = '\0';
    return q;
}
CacheResponse newCacheResponse(char* buf, int len) {
    int i;
    CacheResponse ret;
    ret = (CacheResponse)malloc(sizeof(struct cache_response_t));

    char* tdat;

    buf = fetch_str(buf, &tdat);
    ret->retcode = (int)strtol(tdat, NULL, 10);
    free(tdat);

    buf = fetch_str(buf, &tdat);
    ret->message = tdat;

    buf = fetch_str(buf, &tdat);
    ret->ncols   = (int)strtol(tdat, NULL, 10);
    free(tdat);

    buf = fetch_str(buf, &tdat);
    ret->nrows   = (int)strtol(tdat, NULL, 10);
    free(tdat);

    ret->headers = (char**)malloc(sizeof(char*)*ret->ncols);
    for(i=0; i<ret->ncols; i++) {
        buf = fetch_str(buf, &tdat);
        ret->headers[i] = tdat;
    }
    ret->data = (char**)malloc(sizeof(char*)*ret->ncols*ret->nrows);
    for(i=0; i<ret->ncols*ret->nrows; i++) {
        buf = fetch_str(buf, &tdat);
        ret->data[i] = tdat;
    }

    return ret;
}

int freeCacheResponse(CacheResponse r) {
    if(r==NULL) { return 1; }
    int i;
    for(i=0; i<r->ncols; i++) {
        free(r->headers[i]);
    }
    free(r->headers);
    for(i=0; i<r->ncols*r->nrows; i++) {
        free(r->data[i]);
    }
    free(r->data);
    free(r->message);
    return 0;
}

int cache_response_retcode(CacheResponse r) {
    return r->retcode;
}
char* cache_response_msg(CacheResponse r) {
    return r->message;
}
int cache_response_ncols(CacheResponse r) {
    return r->ncols;
}
int cache_response_nrows(CacheResponse r) {
    return r->nrows;
}
char* cache_response_header(CacheResponse r, int col) {
    if(col>=0 && col<r->ncols) {
        return r->headers[col];
    }
    return NULL;
}
char* cache_response_data(CacheResponse r, int row, int col) {
    if(col<0 || col>=r->ncols) { return NULL; }
    if(row<0 || row>=r->nrows) { return NULL; }
    return r->data[row*r->ncols+col];
}

void print_cache_response(CacheResponse r, FILE* fd) {
    int i,j;
    if(r==NULL) { fprintf(fd, "Bad CacheResponse\n"); return; }

    fprintf(fd, "%d | %s | %d | %d",r->retcode, r->message, r->ncols, r->nrows);
    for(i=0;i<r->ncols;i++) {
        fprintf(fd, "%s |", r->headers[i]);
    }
    fprintf(fd, "\n");

    for(i=0;i<r->nrows;i++) {
        for(j=0; j<r->ncols; j++) {
            fprintf(fd, "%s |", r->data[i*r->ncols+j]);
        }
        fprintf(fd, "\n");
    }
}

/* Communication */
static RpcConnection rpc;
static RpcService rps;
static HashMap* service_functions;

static char lhost[128];
static unsigned short lport;
static char MY_SERVICE_NAME[]="autozone";

static pthread_t serviceThread;

static void _service_handler(void* args) {
    CacheResponse resp;
    char* rq;
    void* qb = (void*)malloc(BUFLEN);
    RpcEndpoint ep;
    int rc,len,err;
    char* buf1=(char*)malloc(sizeof(char)*BUFLEN);
    char retOK[]="OK";
    AutomataHandler_t ahandle;
    for(;;) {
        printf("waiting on rpc_query...\n");
        len = rpc_query(rps, &ep, buf1, BUFLEN);
        if(len==0) {
            fprintf(stderr,"query error\n");
            exit(1);
        }
        rc = rpc_response(rps, &ep, retOK, sizeof(retOK));
        if(rc==0) {
            fprintf(stderr,"response error\n");
            exit(1);
        }

        resp = newCacheResponse(buf1, BUFLEN);

        char* id = cache_response_msg(resp);
        rc = hm_get(service_functions, id, (void**)&ahandle);
        if(rc==0) {
            fprintf(stderr,"no handler function for %d\n",id);
            freeCacheResponse(resp);
            continue;
        }
        err = ahandle(resp);
        if(err) {
            fprintf(stderr, "handler for %s had a bad time\n",id);
        }
        freeCacheResponse(resp);
    }
    free(buf1);
}

int connect_env(char** host, unsigned short* port, char** servicename) {
    *host = getenv("CACHE_HOST");
    *servicename = getenv("CACHE_SERVICE");
    char* portstr = getenv("CACHE_PORT");

    if(*host==NULL || *servicename==NULL || portstr==NULL) {
        return 1;
    }

    *port=(unsigned short)strtol(portstr,NULL,10);
    return 0;
}

int init_cache(char* host, unsigned short port, char* servicename) {
    int err;
    err = !rpc_init(0);
    if(err) {
        printf("rpc_init failed\n");
        return 1;
    }
    service_functions = hm_create(25L, 0.75);

    printf("connection to %s:%d.%s...\n",host,port,servicename);
    rpc = rpc_connect(host, port, servicename, 1l);
    if(rpc==0) {
        printf("rpc_connect failed\n");
        return 1;
    }

    rps = rpc_offer(MY_SERVICE_NAME);
    if(!rps) {
        printf("Offer failed!\n");
        return 1;
    }
    rpc_details(lhost, &lport);
    err = pthread_create(&serviceThread, NULL, (void*)_service_handler, NULL);
    if(err) {
        printf("could not create the thread to handle events\n");
        return 1;
    }
    return 0;
}

int install_automata(char* automata, AutomataHandler_t ahandle) {
    int err,rc;
    int alen;
    char buf[1024];

    // Prep the automata reg string
    char* rq;
    rc = asprintf(&rq, "SQL:register \"%s\" %s %hu %s",automata,lhost,lport,MY_SERVICE_NAME);
    if(rc<1) {
        fprintf(stderr, "Can't install the automata. Can't generate the query string.\n");
        return 1;
    }
    Q_Decl(query, rc+1);
    memcpy(query,rq,rc+1);
    free(rq);

    // Install the automata
    rc = rpc_call(rpc, Q_Arg(query), strlen(query)+1, buf, sizeof(buf), &alen);
    if(rc==0) {
        fprintf(stderr,"Registration call failed\n");
        return(1);
    }

    // Update the accounting for automata events
    CacheResponse res;
    res = newCacheResponse(buf, sizeof(buf));
    printf("%s\n",buf);
    if(res==NULL || cache_response_retcode(res) != 0) {
        fprintf(stderr,"can't process the registration return for:\n%s--\n",query);
        return 1;
    }
    rc = hm_put(service_functions, cache_response_msg(res), ahandle, NULL);
    freeCacheResponse(res);

    return 0;
}

int install_file_automata(char* fname, AutomataHandler_t ahandle) {
    int result;
    FILE* f;
    int rlen;

    struct stat st;
    stat(fname, &st);
    int buflen=st.st_size;
    char* buf=(char*)malloc(sizeof(char)*(buflen+1));

    f = fopen(fname,"r");
    rlen = fread(buf, buflen, buflen, f);
    if(rlen != buflen) {
        fprintf(stderr,"Problem reading the automata from disk.\n");
        return 1;
    }
    buf[buflen]='\0';
    /*strip whitespace since it is meaningful to the protocol*/
    int i;
    for(i=0;i<buflen;i++) {
        if(buf[i]=='\n') { buf[i]=' '; }
    }
    result = install_automata(buf, ahandle);
    free(buf);

    return result;
}

CacheResponse raw_sql(char* query_text) {
    int rc;
    int len;
    int alen;
    alen = strlen(query_text);

    int rlen=4096;
    char rbuf[rlen];

    Q_Decl(equery, alen+5);
    snprintf(equery, alen+5, "SQL:%s", query_text);
    rc = rpc_call(rpc, Q_Arg(equery), strlen(equery)+1, rbuf, rlen, &len);
    if(rc==0) {
        printf("raw query failed catastrophically\n");
        return NULL;
    }
    return newCacheResponse(rbuf, len);
}

CacheResponse file_sql(char* fname) {
    CacheResponse result;
    FILE* f;
    int slen;

    struct stat st;
    stat(fname, &st);
    int buflen=st.st_size;
    char* buf=(char*)malloc(sizeof(char)*(buflen+1));

    f = fopen(fname,"r");
    slen = fread(buf, buflen, buflen, f);
    if(slen != buflen) {
        fprintf(stderr,"Problem reading the query from disk.\n");
        return NULL;
    }
    buf[buflen]='\0';
    /*strip whitespace since it is meaningful to the protocol*/
    int i;
    for(i=0;i<buflen;i++) {
        if(buf[i]=='\n') { buf[i]=' '; }
    }
    result = raw_sql(buf);
    free(buf);

    return result;
}
