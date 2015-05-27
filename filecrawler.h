/*
 * filecrawler.h
 */
#ifndef HW_FILECRAWLER_H
#define HW_FILECRAWLER_H

#include "sqlstmts.h"
#include "rtab.h"

#include "map.h"

#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>

typedef struct filecrawler {
    Ndx *n;
    struct stat sb;
    int empty;
    int *drop;
} Filecrawler;

Filecrawler *filecrawler_new(char *tablename);

void filecrawler_free(Filecrawler *fc);

void filecrawler_window(Filecrawler *fc, sqlwindow *win);

void filecrawler_filter(Filecrawler *fc, T *t, int nfilters,
                        sqlfilter **filters, int filtertype);

void filecrawler_project (Filecrawler *fc, T *t, Rtab *results);

long long filecrawler_count (Filecrawler *fc); /* count non-dropped */

void filecrawler_reset (Filecrawler *fc); /* clear dropped */

long long filecrawler_dump (Filecrawler *fc);

#endif

