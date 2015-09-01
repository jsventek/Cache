#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "cache.h"
#include <srpc/srpc.h>
#include <adts/hashmap.h>

#define BUFLEN 1024

typedef int (*AutomataHandler_t)(Rtab* resp);

static RpcConnection rpc;
static RpcService rps;
static HashMap* service_functions;

static char* lhost;
static unsigned short lport;
static char MY_SERVICE_NAME[]="autozone";

static pthread_t serviceThread;

static void _service_handler(void* args) {
    Rtab* resp;
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

        resp = rtab_unpack(buf1, BUFLEN);

        char* id = rtab_message(resp);
        rc = hm_get(service_functions, id, (void**)&ahandle);
        if(rc==0) {
            fprintf(stderr,"no handler function for %d\n",id);
            rtab_free(resp);
            continue;
        }
        err = ahandle(resp);
        if(err) {
            fprintf(stderr, "handler for %d had a bad time\n",id);
        }
        rtab_free(resp);
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

    printf("connection to %s:%d.%s...\n",host,port,servicename);
    rpc = rpc_connect(host, port, servicename, 1l);
    if(rpc==0) {
        printf("rpc_connect failed\n");
        return 1;
    }

    rps = rpc_offer((char*)MY_SERVICE_NAME);
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
    rc = asprintf(rq, "SQL:register \"%s\" %s %d %s",automata,lhost,lport,MY_SERVICE_NAME);
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
    Rtab* res;
    res = rtab_new(buf, sizeof(buf));
    if(err) {
        fprintf(stderr,"can't process the registration return\n");
        return 1;
    }
    rc = hm_put(service_functions, rtab_message(res), ahandle, NULL);
    rtab_free(res);

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

int raw_sql(char* query_text, char* rbuf, int rlen) {
    int rc;
    int len;
    int alen;
    alen = strlen(query_text);

    Q_Decl(equery, alen+5);
    snprintf(equery, alen+5, "SQL:%s", query_text);
    rc = rpc_call(rpc, Q_Arg(equery), strlen(equery)+1, rbuf, rlen, &len);
    if(rc==0) {
        printf("raw query failed catastrophically\n");
        return 1;
    }
    return 0;
}

int file_sql(char* fname, char* rbuf, int rlen) {
    int result;
    FILE* f;
    int slen;

    struct stat st;
    stat(fname, &st);
    int buflen=st.st_size;
    char* buf=(char*)malloc(sizeof(char)*(buflen+1));

    f = fopen(fname,"r");
    slen = fread(buf, buflen, buflen, f);
    if(slen != buflen) {
        fprintf(stderr,"Problem reading the automata from disk.\n");
        return 1;
    }
    buf[buflen]='\0';
    /*strip whitespace since it is meaningful to the protocol*/
    int i;
    for(i=0;i<buflen;i++) {
        if(buf[i]=='\n') { buf[i]=' '; }
    }
    result = raw_sql(buf, rbuf, rlen);
    free(buf);

    return result;
}
