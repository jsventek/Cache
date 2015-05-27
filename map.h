#ifndef __HWDB_INDEX_MAP__
#define __HWDB_INDEX_MAP__

#include "hashtable.h"

#include "timestamp.h"

#include "linkedlist.h"

#include "sqlstmts.h"
#include "rtab.h"

typedef struct ndx {
    tstamp_t timeStamp;
    off_t byteOffset;
} Ndx;

typedef struct _f {
    char *name;
    FILE *fs;
    tstamp_t oldest;
    tstamp_t newest;
    unsigned char indexed;
} F;

F *f_init(char *filename);

int f_index(F *f, char *in, char *out);

typedef struct _t {
    int n;
    char **name;
    int  **type;
    List *files;
    pthread_mutex_t t_mutex;
} T;

FILE *t_file(T *t, tstamp_t tstamp);

void t_closefiles(T *t);

void t_extract_relevant_columns(T *t, sqlselect *select,
                                Rtab *results);

void t_extract_relevant_types(T *t, Rtab *results);

int t_lookup_colindex(T *t, char *name);

typedef struct _map {
    Hashtable *ht;
    pthread_mutex_t *m_mutex;
    char *directory;
} Map;

Map *map_new(char *directory);

void map_destroy(Map *map);

int map_create_table(Map *map, char *tablename, int ncols,
                     char **colnames, int **coltypes);

int map_update_table(Map *map, char *filename);

Rtab *map_build_results(Map *map, char *tablename, sqlselect *select);

int map_load(Map *map, char *cmd);

#endif /* __HWDB_INDEX_MAP__ */

