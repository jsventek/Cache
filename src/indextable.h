/*
 * The Homework Database
 *
 * Authors:
 *    Oliver Sharma and Joe Sventek
 *     {oliver, joe}@dcs.gla.ac.uk
 *
 * (c) 2009. All rights reserved.
 */
#ifndef HWDB_INDEXTABLE_H
#define HWDB_INDEXTABLE_H

#include "sqlstmts.h"
#include "rtab.h"
#include "adts/linkedlist.h"
#include "srpc/srpc.h"
#include "table.h"
#include "node.h"

typedef struct indextable Indextable;

Indextable *itab_new(void);

int itab_create_table(Indextable *itab, char *tablename, int ncols,
                      char **colnames, int **coltypes, short tabletype, short primary_column);

int itab_update_table(Indextable *itab, sqlupdate *update);

int itab_is_compatible(Indextable *itab, char *tablename,
                       int ncols, int **coltypes);

int itab_table_exists(Indextable *itab, char *tablename);

Table *itab_table_lookup(Indextable *itab, char *tablename);

int itab_colnames_match(Indextable *itab, char *tablename, sqlselect *select);

Rtab *itab_build_results(Indextable *itab, char *tablename, sqlselect *select);

Rtab *itab_showtables(Indextable *itab);

void itab_lock(Indextable *itab);
void itab_unlock(Indextable *itab);

void itab_lock_table(Indextable *itab, char *tablename);
void itab_unlock_table(Indextable *itab, char *tablename);

Node *itab_is_constrained(Indextable *itab, char *tablename, char **colvals);

#endif
