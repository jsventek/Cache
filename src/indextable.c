/*
 * The Homework Database
 *
 * Authors:
 *    Oliver Sharma and Joe Sventek
 *     {oliver, joe}@dcs.gla.ac.uk
 *
 * (c) 2009. All rights reserved.
 */
#include "indextable.h"

#include "tuple.h"
#include "adts/hashmap.h"
#include "table.h"
#include "sqlstmts.h"
#include "util.h"
#include "nodecrawler.h"
#include "typetable.h"
#include "rtab.h"
#include "srpc/srpc.h"
#include "pubsub.h"
#include "topic.h"
#include "ptable.h"

#include <pthread.h>
#include <string.h>
#include <stdlib.h>

struct indextable {
    HashMap *ht;
    pthread_mutex_t *masterlock;
};


Indextable *itab_new(void) {
    Indextable *itab;

    itab = malloc(sizeof(Indextable));
    itab->ht = hm_create(TT_BUCKETS, 0.75);

    /* Init recursive lock */
    itab->masterlock = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(itab->masterlock, NULL);

    return itab;
}

static int create_topic(char *tn, int nc, char **cnames, int **ctypes) {
    char buf[2048], *p;
    int i;
    p = buf;
    p += sprintf(p, "%d tstamp/timestamp", nc+1); /* +1 for leading timestamp */
    for (i = 0; i < nc; i++) {
        p += sprintf(p, " %s/%s", cnames[i], primtype_name[*ctypes[i]]);
    }
    return top_create(tn, buf);
}

int itab_create_table(Indextable *itab, char *tablename, int ncols,
                      char **colnames, int **coltypes, short tabletype, short primary_column) {

    Table *tn;

    if (tabletype && primary_column == -1) {
        errorf("create persistenttable without a primary key.\n");
        return 0;
    }

    if (tabletype && primary_column != 0) {
        errorf("primary key must be the first column.\n");
        return 0;
    }

    itab_lock(itab);

    debugvf("Itab: creating table\n");

    if (! hm_get(itab->ht, tablename, (void **)&tn)) {
        void *dummyVal;

        debugf("Adding new table to master table\n");

        /* Create new table node */
        tn = table_new(ncols, colnames, coltypes);
        table_tabletype(tn, tabletype, primary_column);

        /* Add into hashtable */
        (void)hm_put(itab->ht, strdup(tablename), tn, &dummyVal);
        (void)create_topic(tablename, ncols, colnames, coltypes);
        if (tabletype)
            (void)ptab_create(tablename);

    } else {
        errorf("Table exists. Doing nothing.\n");
        itab_unlock(itab);
        return 0;
    }

    itab_unlock(itab);

    return 1;
}

int itab_update_table(Indextable *itab, sqlupdate *update) {
    Table *tn;
    Nodecrawler *nc;
    int stat;
    debugvf("Itab: updating table\n");
    itab_lock(itab);
    stat = hm_get(itab->ht, update->tablename, (void **)&tn);
    itab_unlock(itab);
    if (! stat) {
        errorf("Table does not exist. Doing nothing.\n");
        return 0;
    }
    /* Lock table */
    table_lock(tn);
    if (! table_persistent(tn)) {
        errorf("Only persistent tables can be updated.\n");
        table_unlock(tn);
        return 0;
    }

    nc = nodecrawler_new(tn->oldest, tn->newest);

    nodecrawler_apply_filter(nc, tn,
                             update->nfilters, update->filters, update->filtertype);

    nodecrawler_update_cols(nc, tn, update);

    /* Reset dropped markers */
    nodecrawler_reset_all_dropped(nc);
    nodecrawler_free(nc);

    nc = nodecrawler_new(tn->oldest, tn->newest);
    nodecrawler_reset_all_dropped(nc);
    nodecrawler_free(nc);

    /* Unlock table */
    table_unlock(tn);

    return 1;
}

int itab_delete_rows(Indextable *itab, sqldelete *delete) {
    Table *tn;
    Nodecrawler *nc;
    int stat;
    debugvf("Itab: deleting rows from table\n");
    itab_lock(itab);
    stat = hm_get(itab->ht, delete->tablename, (void **)&tn);
    itab_unlock(itab);
    if (! stat) {
        errorf("Table does not exist. Doing nothing.\n");
        return 0;
    }
    if (! table_persistent(tn)) {
        errorf("Only persistent tables support delete.\n");
        return 0;
    }

    /* Lock table */
    table_lock(tn);
    nc = nodecrawler_new(tn->oldest, tn->newest);

    nodecrawler_apply_filter(nc, tn, delete->nfilters, delete->filters,
                             delete->filtertype);
    nodecrawler_delete_rows(nc, tn, delete);

    /* Reset dropped markers */
    nodecrawler_reset_all_dropped(nc);
    nodecrawler_free(nc);

    nc = nodecrawler_new(tn->oldest, tn->newest);
    nodecrawler_reset_all_dropped(nc);
    nodecrawler_free(nc);

    /* Unlock table */
    table_unlock(tn);

    return 1;
}


int itab_is_compatible(Indextable *itab, char *tablename, int ncols, int **coltypes) {
    Table *tn;
    int i, stat;

    itab_lock(itab);
    stat = hm_get(itab->ht, tablename, (void **)&tn);
    itab_unlock(itab);

    /* Check table exists */
    if (! stat) {
        errorf("Insert: No such table: %s\n", tablename);
        return 0;
    }

    /* Check number of columns */
    table_lock(tn);
    if (tn->ncols != ncols) {
        errorf("Insert: Not the same number of columns\n");
        table_unlock(tn);
        return 0;
    }

    /* Check column types */
    for (i=0; i < tn->ncols; i++) {
        if (tn->coltype[i] != coltypes[i]) {
            errorf("Insert: Incompatible column type column num: %d\n", i);
            table_unlock(tn);
            return 0;
        }
    }
    table_unlock(tn);

    return 1;

}

/* Check if insert statement violates a primary key constraint.
 *
 * This function should be called after itab_is_compatible, which
 * performs the necessary checks.
 */
Node *itab_is_constrained(Indextable *itab, char *tablename, char **colvals) {

    Table *tn;

    Nodecrawler *nc;
    int key;
    Node *found = NULL;

    itab_lock(itab);
    (void)hm_get(itab->ht, tablename, (void **)&tn);
    itab_unlock(itab);

    table_lock(tn);
    if (table_persistent(tn)) {

        key = table_key(tn);
        /* If the key is the timestamp, ignore under the assumption
         * that all timestamps are unique. In any case, the new re-
         * cord has not been assigned a timestamp yet.
         */
        debugvf("Value at key index is %s\n", colvals[key]);

        nc = nodecrawler_new(tn->oldest, tn->newest);
        found = nodecrawler_find_value(nc, key, colvals[key]);
        nodecrawler_free(nc);

        if (found) {
            /*errorf("Key %s already exists in %s\n", colvals[key],
            	tablename);*/
            table_unlock(tn);
            return found;
        }
    }
    table_unlock(tn);
    debugvf("Table %s is not persistent\n", tablename);
    return NULL;
}

Table *itab_table_lookup(Indextable *itab, char *tablename) {
    Table *ans;
    int stat;

    itab_lock(itab);
    stat = hm_get(itab->ht, tablename, (void **)&ans);
    itab_unlock(itab);
    return (stat) ? ans : NULL;
}

int itab_table_exists(Indextable *itab, char *tablename) {

    return (itab_table_lookup(itab, tablename) != NULL);

}

int itab_colnames_match(Indextable *itab, char *tablename, sqlselect *select) {
    Table *tn;
    int stat;

    itab_lock(itab);
    stat = hm_get(itab->ht, tablename, (void **)&tn);
    itab_unlock(itab);

    /* Check table exists */
    if (! stat) {
        errorf("itab: No such table: %s\n", tablename);
        return 0;
    }

    table_lock(tn);
    if (!table_colnames_match(tn, select)) {
        errorf("Column names in SELECT don't match with this table.\n");
        table_unlock(tn);
        return 0;
    }
    table_unlock(tn);

    return 1;
}

Rtab *itab_build_results(Indextable *itab, char *tablename, sqlselect *select) {
    Table *tn;
    Rtab *results;
    Nodecrawler *nc;
    int stat;

    itab_lock(itab);
    stat = hm_get(itab->ht, tablename, (void **)&tn);
    itab_unlock(itab);

    /* Check table exists */
    if (! stat) {
        errorf("itab: No such table: %s\n", tablename);
        return NULL;
    }

    /* Lock table */
    table_lock(tn);

    /* Build results */
    results = rtab_new();
    table_store_select_cols(tn, select, results);
    table_extract_relevant_types(tn, results);

    /* Now add all relevant rows
     *   -- apply_window
     *   -- apply_filter
     *   -- project columns
     *
     * Note that the basic idea here is to manipulate a list
     * of tuples, running over it and dropping tuples that
     * don't pass the window or filter rules.
     *
     * The tuples that remain in the list are all ok and then
     * the columns are projected from these tuples.
     */
    nc = nodecrawler_new(tn->oldest, tn->newest);
    nodecrawler_apply_window(nc, select->windows[0]); /* NB only one window */
    nodecrawler_apply_filter(nc, tn, select->nfilters, select->filters, select->filtertype);
    nodecrawler_project_cols(nc, tn, results);

    /* group by */
    if (select->groupby_ncols > 0) {
        rtab_groupby(results, select->groupby_ncols, select->groupby_cols,
                     select->isCountStar, select->containsMinMaxAvgSum, select->colattrib);
    } else {
        debugf("Computing count, min, max, avg, sum?\n");
        /* count, min, max, avg, sum */
        if (select->isCountStar) {
            rtab_countstar(results);
        } else if (select->containsMinMaxAvgSum) {
            rtab_processMinMaxAvgSum(results, select->colattrib);
        }
    }

    /* order by */
    rtab_orderby(results, select->orderby);

    /* Reset dropped markers */
    nodecrawler_reset_all_dropped(nc);

    nodecrawler_free(nc);


    /* Unlock table */
    table_unlock(tn);

    return results;
}

Rtab *itab_showtables(Indextable *itab) {
    Rtab *results;
    LinkedList *rowlist;
    char **tnames;
    Rrow *r;
    int j;
    long N;
    long dummyLong;

    itab_lock(itab);

    tnames = hm_keyArray(itab->ht, &N);
    if (! tnames) {
        results = rtab_new_msg(RTAB_MSG_NO_TABLES_DEFINED, NULL);
        itab_unlock(itab);
        return results;
    }
    results = rtab_new();
    results->ncols = 1;
    results->nrows = N;
    results->colnames = (char **)malloc(sizeof(char *));
    results->colnames[0] = strdup("Tablename");
    results->coltypes = (int **)malloc(sizeof(int *));
    results->coltypes[0] = PRIMTYPE_VARCHAR;
    rowlist = ll_create();
    for (j = 0; j < results->nrows; j++) {
        r = malloc(sizeof(Rrow));
        r->cols = malloc(sizeof(char *));
        r->cols[0] = strdup(tnames[j]);
        (void)ll_add(rowlist, r);
    }
    results->rows = (Rrow **)ll_toArray(rowlist, &dummyLong);
    ll_destroy(rowlist, NULL);
    free(tnames);
    itab_unlock(itab);

    return results;
}

void itab_lock(Indextable *itab) {
    debugf("Itab: Acquiring masterlock...\n");
    pthread_mutex_lock(itab->masterlock);
}

void itab_unlock(Indextable *itab) {
    debugf("Itab: Releasing masterlock...\n");
    pthread_mutex_unlock(itab->masterlock);
}

void itab_lock_table(Indextable *itab, char *tablename) {
    Table *tn;
    int stat;

    debugf("Itab locking table: %s\n", tablename);

    itab_lock(itab);
    stat = hm_get(itab->ht, tablename, (void *)&tn);
    itab_unlock(itab);

    if (! stat) {
        errorf("itab: No such table: %s\n", tablename);
        return;
    }

    table_lock(tn);
}

void itab_unlock_table(Indextable *itab, char *tablename) {
    Table *tn;
    int stat;

    debugf("Itab unlocking table: %s\n", tablename);

    itab_lock(itab);
    stat = hm_get(itab->ht, tablename, (void **)&tn);
    itab_unlock(itab);

    if (! stat) {
        errorf("itab: No such table: %s\n", tablename);
        return;
    }

    table_unlock(tn);
}
