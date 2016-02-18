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
