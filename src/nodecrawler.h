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

/**
 * nodecrawler.h
 *
 * Node Crawler
 *
 * The basic idea here is to manipulate a dlist
 * of tuples, running over it and dropping tuples (Nodes) that
 * don't pass the window or filter rules.
 *
 * The tuples that remain in the list are all ok and then
 * the columns are projected from these tuples.
 *
 * To avoid expensive duplicating of list nodes, a field "dropped"
 * was added to typedef node. When set to true, the node is considered
 * dropped. Cleanup must be done on the list resetting all of these fields.
 *
 * Note that this implies that only one crawler can work on a list at any time.
 *
 * Created by Oliver Sharma on 2009-05-06
 */
#ifndef HW_NODECRAWLER_H
#define HW_NODECRAWLER_H

#include "node.h"
#include "sqlstmts.h"
#include "rtab.h"
#include "table.h"


typedef struct nodecrawler {

    /* Set window boundaries */
    Node *first;
    Node *last; /* last is last added (i.e newest) */
    Node *current;
    int empty;
} Nodecrawler;

Nodecrawler *nodecrawler_new(Node *first, Node *last);
Nodecrawler *nodecrawler_new_from_window(Table *tbl, sqlwindow *win);

void nodecrawler_free(Nodecrawler *nc);

void nodecrawler_apply_window(Nodecrawler *nc, sqlwindow *win);
void nodecrawler_apply_filter(Nodecrawler *nc, Table *tn, int nfilters,
                              sqlfilter **filters, int filtertype);

void nodecrawler_project_cols(Nodecrawler *nc, Table *tn, Rtab *results);

/* points current to first node
 */
void nodecrawler_set_to_start(Nodecrawler *nc);


/* returns TRUE if current node is last->next
 */
int nodecrawler_has_more(Nodecrawler *nc);

/* Moves to next node in list where dropped is false
 *
 * NB: always check with nodecrawler_not_at_end() first
 */
void nodecrawler_move_to_next(Nodecrawler *nc);

int nodecrawler_count_nondropped(Nodecrawler *nc);

void nodecrawler_reset_all_dropped(Nodecrawler *nc);

void nodecrawler_update_cols(Nodecrawler *nc, Table *tn, sqlupdate *update);

void nodecrawler_delete_rows(Nodecrawler *nc, Table *tn, sqldelete *delete);

Node *nodecrawler_find_value(Nodecrawler *nc, int key, char *value);

#endif
