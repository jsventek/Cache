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

/* table.c
 *
 * Represents a Table in the database
 *
 * Created by Oliver Sharma on 2009-05-05.
 */
#include "table.h"

#include "util.h"
#include "typetable.h"
#include "sqlstmts.h"
#include "pubsub.h"
#include "srpc/srpc.h"

#include <string.h>
#include <pthread.h>
#include <stdlib.h>

Table *table_new(int ncols, char **colname, int **coltype) {
    Table *tn;
    int i;

    tn = malloc(sizeof(Table));
    tn->ncols = ncols;
    tn->colname = (char **)malloc(ncols * sizeof(char *));
    tn->coltype = (int **)malloc(ncols * sizeof(int *));
    for (i = 0; i < ncols; i++) {
        tn->colname[i] = strdup(colname[i]);
        tn->coltype[i] = coltype[i];
    }
    tn->oldest = NULL;
    tn->newest = NULL;
    tn->count = 0;
    pthread_mutex_init(&tn->tb_mutex, NULL);

    return tn;
}

static int table_contains_col(Table *tn, char *colname) {
    int i;

    for (i = 0; i < tn->ncols; i++) {
        if (strcmp(tn->colname[i], colname) == 0) {
            debugvf("Col %s exists in table. Good.\n", colname);
            return 1;
        }
    }
    if (strcmp(colname, "timestamp") == 0)
        return 1;
    return 0;
}

int table_colnames_match(Table *tn, sqlselect *select) {
    int i;
    char *colname;

    /* Special case: SELECT * from TABLE */
    if (select->ncols == 1 && strcmp(select->cols[0],"*")==0) {
        debugvf("Selecting *, so colums match by default.\n");
        return 1;
    }
    for (i=0; i < select->ncols; i++) {
        colname = select->cols[i];
        if (!table_contains_col(tn, colname)) {
            errorf("No such column: %s\n", colname);
            return 0;
        }
    }
    return 1;
}

void table_lock(Table *tn) {
    debugf("Table: Acquiring lock...\n");
    pthread_mutex_lock(&tn->tb_mutex);
}

void table_unlock(Table *tn) {
    debugf("Table: Releasing lock...\n");
    pthread_mutex_unlock(&tn->tb_mutex);
}

void table_store_select_cols(Table *tn, sqlselect *select, Rtab *results) {
    int i;
    int isselstar;
    int ncols;
    char *p;

    /* check if select *, if so duplicate column names from table
     * otherwise, duplicate column names from select structure */
    isselstar = (select->ncols == 1 && strcmp(select->cols[0], "*") == 0);
    if (isselstar)
        ncols = tn->ncols + 1;	/* +1 for timestamp */
    else
        ncols = select->ncols;
    results->colnames = (char **)malloc(ncols * sizeof(char *));
    results->ncols = ncols;
    for (i = 0; i < ncols; i++) {
        if (isselstar)
            if (i == 0)
                p = "timestamp";
            else
                p = tn->colname[i-1];
        else
            p = select->cols[i];
        results->colnames[i] = strdup(p);
    }
}

/* Returns NULL if not found */
static int *table_lookup_coltype(Table *tn, char *colname) {
    int i;

    for (i=0; i < tn->ncols; i++) {
        if (strcmp(tn->colname[i], colname) == 0) {
            debugvf("Col %s exists and has type: %s\n",
                    colname, primtype_name[*tn->coltype[i]]);
            return tn->coltype[i];
        }
    }
    if (strcmp(colname, "timestamp") == 0)
        return PRIMTYPE_TIMESTAMP;

    return NULL;
}

void table_extract_relevant_types(Table *tn, Rtab *results) {
    int i;

    if (results->ncols>0) {
        results->coltypes = malloc(results->ncols * sizeof(int*));
        for (i=0; i<results->ncols; i++) {
            results->coltypes[i] = table_lookup_coltype(tn,
                                   results->colnames[i]);
        }
    }
}

/* Returns -1 if no such colname */
int table_lookup_colindex(Table *tn, char *colname) {
    int i;

    for (i = 0; i < tn->ncols; i++) {
        if (strcmp(tn->colname[i], colname) == 0) {
            debugvf("Col %s exists and has index: %d\n", colname, i);
            return i;
        }
    }

    /* Not found */
    return -1;
}

void table_tabletype(Table *tn, short tabletype, short primary_column) {
    tn->tabletype = tabletype;
    tn->primary_column = primary_column;
}

int table_persistent(Table *tn) {
    return (tn->tabletype);
}
int table_key(Table *tn) {
    return (tn->primary_column);
}
