/**
 * nodecrawler.h
 *
 * Node Crawler
 *
 * Created by Oliver Sharma on 2009-05-06
 * Copyright (c) 2009. All rights reserved.
 */
#include "nodecrawler.h"

#include "util.h"
#include "adts/linkedlist.h"
#include "rtab.h"
#include "sqlstmts.h"
#include "table.h"
#include "tuple.h"
#include "timestamp.h"
#include "gram.h"

#include "mb.h"

#include <string.h>
#include <sys/time.h>
#include <stdlib.h>

#define set_dropped(node) {\
	(node)->tstamp |= DROPPED; }
#define clr_dropped(node) {\
	(node)->tstamp &= ~DROPPED; }
#define is_dropped(node) ((node)->tstamp & DROPPED)

Nodecrawler *nodecrawler_new(Node *first, Node *last) {
    Nodecrawler *nc;

    nc = malloc(sizeof(Nodecrawler));
    nc->first = first;
    nc->last = last;
    nc->current = first;
    nc->empty = 0; /*false*/

    if (!first && !last) {
        debugvf("Nodecrawler: empty list!\n");
        nc->empty = 1; /*true*/
    }

    return nc;
}

static void nodecrawler_reset(Nodecrawler *nc, Table *tbl) {
    nc->first = tbl->oldest;
    nc->last = tbl->newest;
    nc->current = tbl->oldest;
    nc->empty = 0; /* false */

    if (!nc->first && !nc->last) {
        debugvf("Nodecrawler: empty list!\n");
        nc->empty = 1; /*true*/
    }
    return;
}

Nodecrawler *nodecrawler_new_from_window(Table *tbl, sqlwindow *win) {
    Nodecrawler *nc;

    nc = nodecrawler_new(tbl->oldest, tbl->newest);

    nodecrawler_apply_window(nc, win);

    return nc;
}

void nodecrawler_free(Nodecrawler *nc) {
    free(nc);
}

void nodecrawler_apply_tuplewindow(Nodecrawler *nc, sqlwindow *win) {
    int i;
    Node *tmp;

    if (nc->empty) {
        debugvf("Nodecrawler: empty list! (Doing nothing)\n");
        return ;
    }

    if (win->num < 0) {
        errorf("Nodecrawler: window with invalid row number: %d\n", win->num);
        return;
    }

    tmp = nc->last;

    i = win->num;
    while (i>1 && tmp->prev != NULL) {
        debugvf("Nodecrawler: moving first window back by one\n");
        tmp = tmp->prev;
        i--;
    }

    /* Reached last now. Since we're going backwards, this will be first */
    nc->first = tmp;

    /* Might as well reset current at this stage */
    nodecrawler_set_to_start(nc);
}

/*
 * return true if "ts op then" is true, false otherwise
 */
static int compts(int op, tstamp_t ts, tstamp_t then) {
    int ans;

    switch(op) {
    case LESS:
        ans = (ts < then);
        break;
    case LESSEQ:
        ans = (ts <= then);
        break;
    case GREATER:
        ans = (ts > then);
        break;
    case GREATEREQ:
        ans = (ts >= then);
        break;
    default:
        ans = 0;
        break;
    }
    return ans;
}

/*
 * scan linked list backwards for first node that satisfies tstamp inequality
 */
static Node *first_backward(int op, Nodecrawler *nc, tstamp_t then) {
    Node *tmp, *ans, *sentinel;

    tmp = nc->last;
    if (! compts(op, tmp->tstamp, then))
        return NULL;
    sentinel = nc->first->prev;
    ans = nc->first;		/* assume that all fit in the window */
    tmp = tmp->prev;
    while (tmp != sentinel) {
        if (! compts(op, tmp->tstamp, then)) {
            ans = tmp->next;	/* went one too far */
            break;
        }
        tmp = tmp->prev;
    }
    return ans;
}

/*
 * scan linked list forwards for first node that satisfies tstamp inequality
 */
static Node *first_forward(int op, Nodecrawler *nc, tstamp_t then) {
    Node *tmp, *ans, *sentinel;

    tmp = nc->first;
    if (! compts(op, tmp->tstamp, then))
        return NULL;
    sentinel = nc->last->next;
    ans = nc->last;		/* assume that all fit in the window */
    tmp = tmp->next;
    while (tmp != sentinel) {
        if (! compts(op, tmp->tstamp, then)) {
            ans = tmp->prev;	/* went one too far */
            break;
        }
        tmp = tmp->next;
    }
    return ans;
}

/*
 * locates first tuple in the table that satisfies "Node->tstamp op then"
 *
 * returns pointer to the Node (or NULL)
 */
static Node *first_tuple(int op, Nodecrawler *nc, tstamp_t then) {

    if (nc->empty)
        return NULL;
    if (op == GREATER || op == GREATEREQ)
        return first_backward(op, nc, then);
    else
        return first_forward(op, nc, then);
}

void nodecrawler_apply_sincewindow(Nodecrawler *nc, sqlwindow *win) {
    Node *tmp;

    tmp = first_tuple(GREATER, nc, win->tstampv);
    if (! tmp) {
        debugf("Nodecrawler: nothing fits into this since window.\n");
        nc->first = nc->last;
        nc->current = nc->last;
        nc->empty = 1;
        return;
    }
    nc->first = tmp;
    nodecrawler_set_to_start(nc);
}

void nodecrawler_apply_intervalwindow(Nodecrawler *nc, sqlwindow *win) {
    Node *tmp;

    tmp = first_tuple((win->intv).leftOp, nc, (win->intv).leftTs);
    if (! tmp) {
        debugvf("Nodecrawler: Nothing fits into this interval window.\n");
        nc->first = nc->last;
        nc->current = nc->last;
        nc->empty = 1;
        return;
    }
    /*
     * tmp points to first tuple in the interval window
     */
    nc->first = tmp;
    tmp = first_tuple((win->intv).rightOp, nc, (win->intv).rightTs);
    if (! tmp) { /* otherwise, it will crash */
        debugvf("Nodecrawler: Nothing fits into this interval window.\n");
        nc->first = nc->last;
        nc->current = nc->last;
        nc->empty = 1;
        return;
    }
    /*
     * tmp points to last tuple in the interval window
     */
    nc->last = tmp;

    nodecrawler_set_to_start(nc);
}

void nodecrawler_apply_timewindow(Nodecrawler *nc, sqlwindow *win) {
    struct timeval now;
    Node *tmp;
    int units;
    int ifmillis = 0;
    tstamp_t nowts, thents;

    /* Get current time */
    if (gettimeofday(&now, NULL) != 0) {
        errorf("gettimeofday() failed. Unable to apply time window\n");
        return;
    }
    nowts = timeval_to_timestamp(&now);

    /* Convert given units into seconds or milliseconds */
    switch (win->unit) {

        /* Special case NOW window (i.e. only tuples with equivalent timestamp)*/
    case SQL_WINTYPE_TIME_NOW:
        units = 0;
        break;

    case SQL_WINTYPE_TIME_HOURS:
        units = win->num * 3600;
        break;

    case SQL_WINTYPE_TIME_MINUTES:
        units = win->num * 60;
        break;

    case SQL_WINTYPE_TIME_SECONDS:
        units = win->num;
        break;

    case SQL_WINTYPE_TIME_MILLIS:
        units = win->num;
        ifmillis = 1;
        break;

    default:
        errorf("Unknown unit format in nodecrawler_apply_timewindow");
        return;
        break;

    }

    thents = timestamp_sub_incr(nowts, units, ifmillis);

    /* find first tuple in the list that fits in the window */
    tmp = first_tuple(GREATEREQ, nc, thents);
    if (! tmp) {
        debugf("Nodecrawler: Nothing fits into this range window.\n");
        nc->first=nc->last;
        nc->current=nc->last;
        nc->empty = 1; /*true*/
        return;
    }

    nc->first = tmp;

    /* Might as well reset current at this stage */
    nodecrawler_set_to_start(nc);
}

void nodecrawler_apply_window(Nodecrawler *nc, sqlwindow *win) {

    if (nc->empty) {
        debugvf("Nodecrawler: empty list! (Doing nothing)\n");
        return ;
    }

    switch(win->type) {

    case SQL_WINTYPE_TPL:
        debugvf("Nodecrawler: wintype_tpl\n");
        nodecrawler_apply_tuplewindow(nc, win);
        break;

    case SQL_WINTYPE_TIME:
        debugvf("Nodecrawler: wintype_time\n");
        nodecrawler_apply_timewindow(nc, win);
        break;

    case SQL_WINTYPE_SINCE:
        debugvf("Nodecrawler: wintype_since\n");
        nodecrawler_apply_sincewindow(nc, win);
        break;

    case SQL_WINTYPE_INTERVAL:
        debugvf("Nodecrawler: wintype_interval\n");
        nodecrawler_apply_intervalwindow(nc, win);
        break;

    case SQL_WINTYPE_NONE:
        debugvf("Nodecrawler: wintype_none. doing nothing\n");
        break;

    default:
        errorf("Nodecrawler encountered invalid window type\n");
        break;
    }

}

static int compare(int op, void *cVal, int *cType, union filterval *filVal) {
    if (cType == PRIMTYPE_INTEGER) {
        long long val = strtoll((char *)cVal, NULL, 10);
        switch(op) {
        case SQL_FILTER_EQUAL:
            return (val == filVal->intv);
            break;
        case SQL_FILTER_GREATER:
            return (val > filVal->intv);
            break;
        case SQL_FILTER_LESS:
            return (val < filVal->intv);
            break;
        case SQL_FILTER_LESSEQ:
            return (val <= filVal->intv);
            break;
        case SQL_FILTER_GREATEREQ:
            return (val >= filVal->intv);
            break;
        }
    } else if (cType == PRIMTYPE_REAL) {
        double val = strtod((char *)cVal, NULL);
        switch(op) {
        case SQL_FILTER_EQUAL:
            return (val == filVal->realv);
            break;
        case SQL_FILTER_GREATER:
            return (val > filVal->realv);
            break;
        case SQL_FILTER_LESS:
            return (val < filVal->realv);
            break;
        case SQL_FILTER_LESSEQ:
            return (val <= filVal->realv);
            break;
        case SQL_FILTER_GREATEREQ:
            return (val >= filVal->realv);
            break;
        }
    } else if (cType == PRIMTYPE_VARCHAR) {
        char *val = (char *)cVal;
        debugvf("compare %s with %s\n", val, filVal->stringv);
        switch(op) {
        case SQL_FILTER_EQUAL:
            return (strcmp(val, filVal->stringv) == 0);
            break;
        case SQL_FILTER_CONTAINS:
            return (strstr(val, filVal->stringv) != NULL);
            break;
        case SQL_FILTER_NOTCONTAINS:
            return (strstr(val, filVal->stringv) == NULL);
            break;
            /* implement >, < on lexicographical order */
        }
    } else if (cType == PRIMTYPE_TIMESTAMP) {
        unsigned long long val = *(tstamp_t *)cVal;
        switch(op) {
        case SQL_FILTER_EQUAL:
            return (val == filVal->tstampv);
            break;
        case SQL_FILTER_GREATER:
            return (val > filVal->tstampv);
            break;
        case SQL_FILTER_LESS:
            return (val < filVal->tstampv);
            break;
        case SQL_FILTER_LESSEQ:
            return (val <= filVal->tstampv);
            break;
        case SQL_FILTER_GREATEREQ:
            return (val >= filVal->tstampv);
            break;
        }
    }
    return 0;			/* false if we get here */
}

static char *updatevalue(int op, void *cVal, int *cType, union filterval *filVal) {
    char r[256];
    memset(r, 0, sizeof(r));
    if (cType == PRIMTYPE_INTEGER) {
        long long val = strtoll((char *)cVal, NULL, 10);
        long long new = filVal->intv;
        debugvf("update: type integer value %lld\n", val);
        switch(op) {
        case SQL_PAIR_EQUAL:
            sprintf(r, "%lld", new);
            return strdup(r);
            break;
        case SQL_PAIR_ADDEQ:
            sprintf(r, "%lld", new + val);
            return strdup(r);
            break;
        case SQL_PAIR_SUBEQ:
            sprintf(r, "%lld", val - new);
            return strdup(r);
            break;
        }
    } else if (cType == PRIMTYPE_REAL) {
        double val = strtod((char *)cVal, NULL);
        double new = filVal->realv;
        debugvf("update: type real value %5.2f\n", val);
        switch(op) {
        case SQL_PAIR_EQUAL:
            sprintf(r, "%5.2f", new);
            return strdup(r);
            break;
        case SQL_PAIR_ADDEQ:
            sprintf(r, "%5.2f", new + val);
            return strdup(r);
            break;
        case SQL_PAIR_SUBEQ:
            sprintf(r, "%5.2f", val - new);
            return strdup(r);
            break;
        }
    }
    return NULL;
}

int passed_filter(Node *n, Table *tn, int nfilters, sqlfilter **filters, int filtertype) {
    int i;
    char *filName;
    union filterval filVal;
    int filSign;
    int colIdx;
    void *colVal;
    int *colType;
    union Tuple *p = (union Tuple *)(n->tuple);

    for (i = 0; i < nfilters; i++) {

        filName = filters[i]->varname;
        memcpy(&filVal, &filters[i]->value, sizeof(union filterval));
        filSign = filters[i]->sign;

        colIdx = table_lookup_colindex(tn, filName);
        if (colIdx == -1)  { /* i.e. invalid variable name */
            if (strcmp(filName, "timestamp") == 0) {
                colVal = (void *)&(n->tstamp);
                colType = PRIMTYPE_TIMESTAMP;
            } else {
                errorf("Invalid column name in filter: %s\n", filName);
                return 1; /* automatically pass this filter */
            }
        } else {
            colType = tn->coltype[colIdx];
            colVal = (void *)(p->ptrs[colIdx]);
        }

        if (filtertype == SQL_FILTER_TYPE_OR) {

            debugf("Filtertype in nodecrawler is OR\n");

            if (compare(filSign, colVal, colType, &filVal)) {
                return 1;
            }

        } else { /* default to AND filter */

            debugf("Filtertype in nodecrawler is AND\n");

            if (! compare(filSign, colVal, colType, &filVal)) {
                return 0;
            }
        }
    }
    /* If we get here, all filters passed; or not */
    if (filtertype == SQL_FILTER_TYPE_OR)
        return 0;
    return 1;
}

void nodecrawler_apply_filter(Nodecrawler *nc, Table *tn, int nfilters,
                              sqlfilter **filters, int filtertype) {

    if (nc->empty) {
        debugvf("Nodecrawler: empty list! (Doing nothing)\n");
        return ;
    }

    nodecrawler_set_to_start(nc);
    while(nodecrawler_has_more(nc)) {

        if (!passed_filter(nc->current, tn, nfilters, filters, filtertype)) {
            set_dropped(nc->current);
        }

        nodecrawler_move_to_next(nc);
    }

}

void nodecrawler_set_to_start(Nodecrawler *nc) {

    if (nc->empty) {
        debugvf("Nodecrawler: empty list! (Doing nothing)\n");
        return ;
    }

    nc->current = nc->first;

    if (is_dropped(nc->current))
        nodecrawler_move_to_next(nc);
}

int nodecrawler_has_more(Nodecrawler *nc) {

    return (!nc->empty && nc->current != nc->last->next);
}

void nodecrawler_move_to_next(Nodecrawler *nc) {

    if (nc->empty) {
        debugvf("Nodecrawler: empty list! (Doing nothing)\n");
        return ;
    }

    if (!nodecrawler_has_more(nc))
        return;

    nc->current = nc->current->next;

    while (nodecrawler_has_more(nc) && is_dropped(nc->current)) {
        nc->current = nc->current->next;
    }
}

/* NB: dangerous method if last does not follow first in chain */
int nodecrawler_count_nondropped(Nodecrawler *nc) {
    Node *tmp;
    int count;

    if (nc->empty) {
        return 0;
    }

    tmp = nc->first;
    count = 0;

    while (tmp != nc->last->next) {

        if (! is_dropped(tmp)) {
            count++;
        }

        tmp = tmp->next;
    }

    return count;
}

void nodecrawler_reset_all_dropped(Nodecrawler *nc) {
    Node *tmp;

    if (nc->empty) {
        debugvf("Nodecrawler: empty list! (Doing nothing)\n");
        return ;
    }

    tmp = nc->first;

    while (tmp != nc->last->next) {
        clr_dropped(tmp);
        tmp = tmp->next;
    }
}

void nodecrawler_project_cols(Nodecrawler *nc, Table *tn, Rtab *results) {
    LinkedList *rowlist;
    Rrow *r;
    int i;
    char *colname;
    int colIdx;
    int len;
    union Tuple *p;
    long dummyLen;

    if (nc->empty) {
        debugvf("Nodecrawler: empty list! (Doing nothing)\n");
        return ;
    }

    debugvf("Nodecrawler: Projecting columns\n");

    rowlist = ll_create();
    nodecrawler_set_to_start(nc);
    while (nodecrawler_has_more(nc)) {
        /* Extract data from tuple */
        r = malloc(sizeof(Rrow));
        r->cols = malloc(results->ncols * sizeof(char*));

        for (i = 0; i < results->ncols; i++) {

            colname = results->colnames[i];

            colIdx = table_lookup_colindex(tn, colname);
            if (colIdx == -1)	/* was timestamp */
                r->cols[i] = timestamp_to_string(nc->current->tstamp);
            else {
                p = (union Tuple *)(nc->current->tuple);
                debugvf("Sanity check: values[%d]=%s\n", colIdx,
                        p->ptrs[colIdx]);
                len = strlen(p->ptrs[colIdx]) + 1;
                r->cols[i] = malloc(len);
                strcpy(r->cols[i], p->ptrs[colIdx]);
            }
            debugvf("r->cols[%d]: %s\n", i, r->cols[i]);
        }

        (void)ll_add(rowlist, r);

        nodecrawler_move_to_next(nc);
    }

    /* Build array from rowlist */
    results->nrows = (int)ll_size(rowlist);
    results->rows = (Rrow**) ll_toArray(rowlist, &dummyLen);
    ll_destroy(rowlist, NULL);
}

char *updatetable(sqlupdate *update, void *colVal, int *colType, int idx,
                  Table *tn) {

    int i, j;
    char *varname;
    union filterval value;
    int sign;

    char *r = NULL;

    debugf("%d pairs in update statement\n", update->npairs);
    for (i = 0; i < update->npairs; i++) { /* for every pair */
        /* variable <operation> value */
        varname = update->pairs[i]->varname;
        memcpy(&value, &update->pairs[i]->value, sizeof(union filterval));
        sign = update->pairs[i]->sign;

        j = table_lookup_colindex(tn, varname); /* find column index */
        /* current index */
        if (j == idx) {
            r = updatevalue(sign, colVal, colType, &value);
            if (r)
                debugvf("updated value for %s is %s\n", varname, r);
        }
    }
    return r;
}

void nodecrawler_delete_rows(Nodecrawler *nc, Table *tn, sqldelete *delete) {
    Node *n;
    union Tuple *p;

    char *value;

    int i;

    void *colVal;
    int *colType;

    LinkedList *lcols;
    int ncols;
    char **colvals;

    if (nc->empty) {
        debugvf("Nodecrawler: empty list! (Doing nothing)\n");
        return;
    }
    debugvf("Nodecrawler: deteleting rows\n");
    nodecrawler_set_to_start(nc);
    while (nodecrawler_has_more(nc)) {
        long dummyLen;
        n = nc->current;

        /* remove n from list */
        if (tn->oldest == tn->newest) { /* == n */
            tn->oldest = NULL;
            tn->newest = NULL;
        } else if (tn->oldest == n) {
            tn->oldest = n->next;
            n->next->prev = NULL;
        } else if (tn->newest == n) {
            tn->newest = n->prev;
            n->prev->next = NULL;
        } else {
            n->prev->next = n->next;
            n->next->prev = n->prev;
        }
        free(n);
        --tn->count;

        /* nc->current = tn->oldest; */
        //nodecrawler_reset(nc, tn);

        nodecrawler_move_to_next(nc);
    }

    return;
}

void nodecrawler_update_cols(Nodecrawler *nc, Table *tn, sqlupdate *update) {
    Node *n, *u;
    union Tuple *p;

    char *value;

    int i;

    void *colVal;
    int *colType;

    LinkedList *lcols;
    int ncols;
    char **colvals;

    if (nc->empty) {
        debugvf("Nodecrawler: empty list! (Doing nothing)\n");
        return;
    }
    debugvf("Nodecrawler: updating columns\n");
    nodecrawler_set_to_start(nc);
    while (nodecrawler_has_more(nc)) {
        long dummyLen;
        n = nc->current;
        p = (union Tuple *)(n->tuple);
        debugvf("node @%p tuple @%p\n", n, p);
        lcols = ll_create();
        if (!lcols)
            return;
        for (i = 0; i < tn->ncols; i++) {
            colVal = (void *)(p->ptrs[i]);
            colType = tn->coltype[i];
            value = updatetable(update, colVal, colType, i, tn);
            if (!value)
                (void)ll_add(lcols, (void *)strdup(colVal));
            else
                (void)ll_add(lcols, (void *)(value));
        }
        ncols = (int)ll_size(lcols);
        if (ncols != tn->ncols) {
            errorf("warning: invalid number of columns: %d (%d)\n",
                   ncols, tn->ncols);
            ll_destroy(lcols, NULL);
            if (value)
                free(value);
            return;
        }
        colvals = (char **) ll_toArray(lcols, &dummyLen);
        ll_destroy(lcols, NULL);
        debugvf("table count %ld\n", tn->count);

        /* remove n from list */
        if (tn->oldest == tn->newest) { /* == n */
            tn->oldest = NULL;
            tn->newest = NULL;
        } else if (tn->oldest == n) {
            tn->oldest = n->next;
            n->next->prev = NULL;
        } else if (tn->newest == n) {
            tn->newest = n->prev;
            n->prev->next = NULL;
        } else {
            n->prev->next = n->next;
            n->next->prev = n->prev;
        }
        --tn->count;

        free(p);
        free(n);

        /* insert */
        u = heap_alloc_node(ncols, colvals, tn);
        if ((tn->count)++) { /* list was not empty */
            tn->newest->next = u;
            u->prev = tn->newest;
            tn->newest = u;
        } else {
            tn->newest = u;
            tn->oldest = u;
        }
        set_dropped(u); /* avoid infinite loop */

        /* if (value)
        	free(value); `value' is free'd below */

        for (i = 0; i < ncols; i++)
            free(colvals[i]);
        free(colvals);

        /* nc->current = tn->oldest; */
        nodecrawler_reset(nc, tn);

        nodecrawler_move_to_next(nc);
    }

    return;
}

Node *nodecrawler_find_value(Nodecrawler *nc, int key, char *value) {

    Node *n;
    union Tuple *p;
    char *cv; /* Value at index 'key'. */

    if (nc->empty) {
        debugvf("Nodecrawler: empty list! (Doing nothing)\n");
        return NULL;
    }

    nodecrawler_set_to_start(nc);
    while(nodecrawler_has_more(nc)) {

        n = nc->current;
        p = (union Tuple *)(n->tuple);
        cv = (char *)(p->ptrs[key]);
        debugvf("In nodecrawler: value at index %d is %s\n", key, cv);
        if (strcmp(cv, value) == 0) return n;
        nodecrawler_move_to_next(nc);
    }
    return NULL;
}
