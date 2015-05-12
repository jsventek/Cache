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
 * table.h - define data structure for tables
 */

#ifndef _TABLE_H_
#define _TABLE_H_

#include "node.h"
#include "linkedlist.h"
#include "sqlstmts.h"
#include "rtab.h"
#include "srpc.h"
#include <pthread.h>

typedef struct table {
    short tabletype;		/* type of table (persistent or not) */
    short primary_column;	/* primary column # for persistent table */
    int ncols;			/* number of columns */
    char **colname;		/* names of columns */
    int **coltype;		/* types of columns */
    struct node *oldest;	/* oldest node in the table */
    struct node *newest;	/* newest node in the table */
    long count;			/* number of nodes in the table */
    pthread_mutex_t tb_mutex;	/* mutex for protecting the table */
} Table;

Table *table_new(int ncols, char **colname, int **coltype);
int table_colnames_match(Table *tn, sqlselect *select);
void table_lock(Table *tn);
void table_unlock(Table *tn);
void table_store_select_cols(Table *tn, sqlselect *select, Rtab *results);
void table_extract_relevant_types(Table *tn, Rtab *results);
int table_lookup_colindex(Table *tn, char *colname);
void table_tabletype(Table *tn, short tabletype, short primary_column);
int table_persistent(Table *tn);
int table_key(Table *tn);

#endif /* _TABLE_H_ */
