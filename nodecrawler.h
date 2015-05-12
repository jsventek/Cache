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
 * Copyright (c) 2009. All rights reserved.
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

Node *nodecrawler_find_value(Nodecrawler *nc, int key, char *value);

#endif
