#ifndef _CACHECONNECT_H_
#define _CACHECONNECT_H_

#include <stdio.h>

/* Types */
typedef struct cache_response_t* CacheResponse;
typedef int (*AutomataHandler_t)(CacheResponse resp);

/* Methods for working with CacheResponses */
CacheResponse newCacheResponse(char* buf, int len);
int freeCacheResponse(CacheResponse r);

int cache_response_retcode(CacheResponse r);
char* cache_response_msg(CacheResponse r);
int cache_response_ncols(CacheResponse r);
int cache_response_nrows(CacheResponse r);
char* cache_response_headers(CacheResponse r, int col);
char* cache_response_data(CacheResponse r, int row, int col);
void print_cache_response(CacheResponse r, FILE* fd);

int connect_env(char** host, unsigned short* port, char** servicename);
int init_cache(char* host, unsigned short port, char* servicename);
int install_automata(char* automata, AutomataHandler_t ahandle);
int install_file_automata(char* fname, AutomataHandler_t ahandle);
CacheResponse raw_sql(char* query_text);
CacheResponse file_sql(char* fname);

#endif
