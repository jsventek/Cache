#ifndef _CACHECONNECT_H_
#define _CACHECONNECT_H_

#include <cache.h>

typedef int (*AutomataHandler_t)(Rtab* resp);

int connect_env(char** host, unsigned short* port, char** servicename);
int init_cache(char* host, unsigned short port, char* servicename);
int install_automata(char* automata, AutomataHandler_t ahandle);
int install_file_automata(char* fname, AutomataHandler_t ahandle);
int raw_sql(char* query_text, char* rbuf, int rlen);
int file_sql(char* fname, char* rbuf, int rlen);

#endif
