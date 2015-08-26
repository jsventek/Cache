/*
 * The Homework Database
 *
 * Authors:
 *    Oliver Sharma and Joe Sventek
 *     {oliver, joe}@dcs.gla.ac.uk
 *
 * (c) 2009. All rights reserved.
 */
#include "hwdb.h"
#include "mb.h"
#include "util.h"
#include "rtab.h"
#include "sqlstmts.h"
#include "parser.h"
#include "indextable.h"
#include "table.h"
#include "adts/hashmap.h"
#include "pubsub.h"
#include "srpc/srpc.h"
#include "adts/tsuqueue.h"
#include "automaton.h"
#include "topic.h"
#include "node.h"
#include "logdefs.h"

#include <stdlib.h>
#include <pthread.h>
#include <string.h>

extern char *progname;
extern void ap_init();

/*
 * static data used by the database
 */
static Indextable *itab;
static int ifUsesRpc = 1;
#ifdef HWDB_PUBLISH_IN_BACKGROUND
static TSUQueue *workQ;
static TSUQueue *cleanQ;
static pthread_t pubthr[NUM_THREADS];
#endif /* HWDB_PUBLISH_IN_BACKGROUND */

/* Used by sql parser */
sqlstmt stmt;

/*
 * forward declarations for functions in this file
 */
Rtab *hwdb_exec_stmt(int isreadonly);
Rtab *hwdb_select(sqlselect *select);
Rtab *hwdb_table_meta(char* tablename);
int hwdb_create(sqlcreate *create);
int hwdb_insert(sqlinsert *insert);
Rtab *hwdb_showtables(void);
int hwdb_register(sqlregister *regist);
int hwdb_unregister(sqlunregister *unregist);
int hwdb_update(sqlupdate *update);
//void hwdb_publish(char *tablename);
int hwdb_send_event(Automaton *au, char *buf, int ifdisconnect);
#ifdef HWDB_PUBLISH_IN_BACKGROUND
void *do_publish(void *args);
#endif /* HWDB_PUBLISH_IN_BACKGROUND */

int hwdb_init(int usesRPC) {

    progname = "cache";
    ifUsesRpc = usesRPC;
    mb_init();
    itab = itab_new();
    top_init();			/* initialize the topic system */
    au_init();			/* initialize the automaton system */
#ifdef HWDB_PUBLISH_IN_BACKGROUND
    int i;
    workQ = tsuq_create();
    cleanQ = tsuq_create();
    for (i = 0; i < NUM_THREADS; i++) {
        pthread_create(&pubthr[i], NULL, do_publish, NULL);
        debugf("Publish thread launched.\n");
    }
#endif /* HWDB_PUBLISH_IN_BACKGROUND */

    return 1;
}

#ifdef HWDB_PUBLISH_IN_BACKGROUND
static void do_cleanup(void) {
    CallBackInfo *cbinfo;

    while (tsuq_remove(cleanQ, (void **)&cbinfo)) {
        au_destroy(au_id(cbinfo->u.au));
        //printf("cleaning up registered automaton\n");
        if (cbinfo->str != NULL)
            free(cbinfo->str);
        free(cbinfo);
    }
}
#endif /* HWDB_PUBLISH_IN_BACKGROUND */

Rtab *hwdb_exec_query(char *query, int isreadonly) {
    void *result;
#ifdef HWDB_PUBLISH_IN_BACKGROUND
    do_cleanup();
#endif /* HWDB_PUBLISH_IN_BACKGROUND */
    result = sql_parse(query);
#ifdef VDEBUG
    sql_print();
#endif /* VDEBUG */
    if (! result)
        return  rtab_new_msg(RTAB_MSG_ERROR, NULL);

    return hwdb_exec_stmt(isreadonly);
}

Rtab *hwdb_exec_stmt(int isreadonly) {
    Rtab *results = NULL;

    switch (stmt.type) {
    case SQL_TABLE_META:
        results = hwdb_table_meta(stmt.sql.meta.table);
        break;
    case SQL_TYPE_SELECT:
        results = hwdb_select(&stmt.sql.select);
        if (!results)
            results = rtab_new_msg(RTAB_MSG_SELECT_FAILED, NULL);
        break;
    case SQL_TYPE_CREATE:
        if (isreadonly || !hwdb_create(&stmt.sql.create)) {
            results = rtab_new_msg(RTAB_MSG_CREATE_FAILED, NULL);
        } else {
            results = rtab_new_msg(RTAB_MSG_SUCCESS, NULL);
        }
        break;
    case SQL_TYPE_INSERT:
        if (isreadonly || !hwdb_insert(&stmt.sql.insert)) {
            results = rtab_new_msg(RTAB_MSG_INSERT_FAILED, NULL);
        } else {
            results = rtab_new_msg(RTAB_MSG_SUCCESS, NULL);
        }
        break;
    case SQL_TYPE_UPDATE:
        if (isreadonly || !hwdb_update(&stmt.sql.update)) {
            results = rtab_new_msg(RTAB_MSG_UPDATE_FAILED, NULL);
        } else {
            results = rtab_new_msg(RTAB_MSG_SUCCESS, NULL);
        }
        break;
    case SQL_TYPE_DELETE:
        if (isreadonly || !hwdb_delete(&stmt.sql.delete)) {
            results = rtab_new_msg(RTAB_MSG_DELETE_FAILED, NULL);
        } else {
            results = rtab_new_msg(RTAB_MSG_SUCCESS, NULL);
        }
        break;
    case SQL_SHOW_TABLES:
        results = hwdb_showtables();
        break;
    case SQL_TYPE_REGISTER: {
        int v;
        if (isreadonly || !(v = hwdb_register(&stmt.sql.regist))) {
            results = rtab_new_msg(RTAB_MSG_REGISTER_FAILED, NULL);
        } else {
            static char buf[20];
            sprintf(buf, "%d", v);
            results = rtab_new_msg(RTAB_MSG_SUCCESS, buf);
        }
        break;
    }
    case SQL_TYPE_UNREGISTER:
        if (isreadonly ||  !hwdb_unregister(&stmt.sql.unregist)) {
            results = rtab_new_msg(RTAB_MSG_UNREGISTER_FAILED, NULL);
        } else {
            results = rtab_new_msg(RTAB_MSG_SUCCESS, NULL);
        }
        break;
    default:
        errorf("Error parsing query\n");
        results = rtab_new_msg(RTAB_MSG_PARSING_FAILED, NULL);
        break;
    }
    reset_statement();
    return results;
}

Rtab *hwdb_select(sqlselect *select) {
    Rtab *results;
    char *tablename;

    debugf("HWDB: Executing SELECT:\n");

    /* Only 1 table supported for now */
    tablename = select->tables[0];

    /* Check table exists */
    if (!itab_table_exists(itab, tablename)) {
        errorf("HWDB: no such table\n");
        return NULL;
    }

    /* if a group-by operator is specified, check its validity */
    if (!sqlstmt_valid_groupby(select)) {
        errorf("HWDB: Invalid group-by operator.\n");
        return NULL;
    }

    /* Check column names match */
    if (!itab_colnames_match(itab, tablename, select)) {
        errorf("HWDB: Column names in SELECT don't match with this table.\n");
        return NULL;
    }

    results = itab_build_results(itab, tablename, select);

    return results;
}

int hwdb_update(sqlupdate *update) {
    debugf("HWDB: Executing UPDATE:\n");
    /* Check table exists */
    if (!itab_table_exists(itab, update->tablename)) {
        errorf("HWDB: %s no such table\n", update->tablename);
        return 0;
    }

    if (itab_update_table(itab, update)) {
        /* hwdb_publish(update->tablename); *//* Notify subscribers */
        return 1;
    }
    return 0;
}

int hwdb_delete(sqldelete *delete) {
    debugf("HWDB: Executing DELETE:\n");
    /* Check table exists */
    if (!itab_table_exists(itab, delete->tablename)) {
        errorf("HWDB: %s no such table\n", delete->tablename);
        return 0;
    }

    if (itab_delete_rows(itab, delete)) {
        return 1;
    }
    return 0;
}

int  hwdb_create(sqlcreate *create) {
    debugf("Executing CREATE:\n");

    return itab_create_table(itab, create->tablename, create->ncols,
                             create->colname, create->coltype, create->tabletype,
                             create->primary_column);
}

static void gen_tuple_string(Table *t, int ncols, char **colvals, char *out) {
    char *p = out;
    char *ts = timestamp_to_string(t->newest->tstamp);
    int i;
    p += sprintf(p, "%s<|>", ts);
    free(ts);
    for (i = 0; i < ncols; i++) {
        p += sprintf(p, "%s<|>", colvals[i]);
    }
}

Rtab* hwdb_table_meta(char* tablename) {
    Table *tn;
    Rrow* row;
    int i;

    debugf("Executing SHOW TABLE %s:\n",tablename);

    if (! (tn = itab_table_lookup(itab, tablename))) {
        errorf("Table name does not exist\n");
        return 0;
    }

    /* retrieve the columns */
    Rtab* results = rtab_new();
    results->ncols=3;
    results->nrows=tn->ncols;
    results->colnames=(char**)malloc(3 * sizeof(char*));
    results->coltypes=(int**)malloc(3 * sizeof(int*));
    results->colnames[0]=strdup("column");
    results->coltypes[0]=PRIMTYPE_VARCHAR;
    results->colnames[1]=strdup("type");
    results->coltypes[1]=PRIMTYPE_VARCHAR;
    results->colnames[2]=strdup("primary key");
    results->coltypes[2]=PRIMTYPE_VARCHAR;
    results->rows=(Rrow**)malloc(results->nrows * sizeof(Rrow*));
    for(i=0; i<results->nrows; i++) {
        row = (Rrow*)malloc(sizeof(Rrow));
        results->rows[i] = row;
        row->cols = (char**)malloc(3 * sizeof(char*));
        row->cols[0] = strdup(tn->colname[i]);
        row->cols[1] = strdup(primtype_name[*(tn->coltype[i])]);
        if(i==tn->primary_column) {
            row->cols[2] = strdup("yes");
        } else {
            row->cols[2] = strdup("");
        }
    }
    
    return results;
}


int  hwdb_insert(sqlinsert *insert) {
    Table *tn;
    char buf[2048];
    Node *n;

    debugf("Executing INSERT:\n");

    if (! (tn = itab_table_lookup(itab, insert->tablename))) {
        errorf("Insert table name does not exist\n");
        return 0;
    }

    /* Check columns are compatible */
    if (!itab_is_compatible(itab, insert->tablename,
                            insert->ncols, insert->coltype)) {
        errorf("Insert not compatible with table\n");
        return 0;
    }

    /* Check values don't violate primary key constraint */
    if ((n = itab_is_constrained(itab, insert->tablename,
                                 insert->colval))) {
        debugf("Transformation %d\n", insert->transform);
        if (! insert->transform) {
            errorf("Insert violates primary key\n");
            return 0;
        }
    }

    /* allocate space for tuple, copy values into tuple, thread new
     * node to end of table */
    if ( table_persistent(tn) ) {
        heap_insert_tuple(insert->ncols, insert->colval, tn, n);
    } else {
        mb_insert_tuple(insert->ncols, insert->colval, tn);
    }
    gen_tuple_string(tn, insert->ncols, insert->colval, buf);
    top_publish(insert->tablename, buf);
    /* Tuple sanity check */
#ifdef DEBUG
#ifdef VDEBUG
    {
        int i;
        union Tuple *p;
        debugvf("SANITY> tuple key: %s\n",insert->tablename);
        p = (union Tuple *)(tn->newest->tuple);
        for (i=0; i < insert->ncols; i++) {
            debugvf("SANITY> colval[%d] = %s\n", i, p->ptrs[i]);
        }
    }
#endif /* VDEBUG */
#endif /* DEBUG */

    /* Notify all subscribers */
    //hwdb_publish(insert->tablename);

    return 1;
}

Rtab *hwdb_showtables(void) {
    debugf("Executing SHOW TABLES\n");
    return itab_showtables(itab);
}

int hwdb_unregister(sqlunregister *unregist) {
    unsigned long id;

    id = strtoul(unregist->id, NULL, 10);
    return au_destroy(id);
}

int hwdb_register(sqlregister *regist) {
    Automaton *au;
    unsigned short port;
    RpcConnection rpc = NULL;
    char buf[10240], *p;
    char ebuf[256];

    debugf("HWDB: Registering an automaton\n");

    /* Open connection to registering process */
    if (ifUsesRpc) {
        port = atoi(regist->port);
        rpc = rpc_connect(regist->ipaddr, port, regist->service, 1l);
        if (! rpc) {
            errorf("Can't connect to callback. Not registering...\n");
            return 0;
        }
    }

    /* compile automaton */
    /* automaton has leading and trailing quotes */
    strcpy(buf, regist->automaton+1);
    p = strrchr(buf, '\"');
    *p = '\0';
    debugf("automaton: %s\n", buf);
    au = au_create(buf, rpc, ebuf);
    if (! au) {
        errorf("Error creating automaton - %s\n", ebuf);
        rpc_disconnect(rpc);
        return 0;
    }

    /* Return the identifier for the automaton */

    return au_id(au);
}

Table *hwdb_table_lookup(char *name) {
    return itab_table_lookup(itab, name);
}

int hwdb_send_event(Automaton *au, char *buf, int ifdisconnect) {
    int result = 0;	/* assume failure */
#ifdef HWDB_PUBLISH_IN_BACKGROUND
    CallBackInfo *cbinfo = (CallBackInfo *)malloc(sizeof(CallBackInfo));
    if (cbinfo) {
        char *s = strdup(buf);
        if (s) {
            cbinfo->type = REGISTRATION;
            cbinfo->ifdisconnect = ifdisconnect;
            cbinfo->str = s;
            cbinfo->u.au = au;
            tsuq_add(workQ, cbinfo);
            result = 1;
        } else {
            errorf("Unable to allocate structure for call back\n");
            free(cbinfo);
        }
    } else {
        errorf("Unable to allocate structure for call back\n");
    }
#else
    /* Send results to registrant */
    char resp[100];
    unsigned rlen;
    int len = strlen(buf) + 1;
    result = rpc_call(au->rpc, buf, len, resp, sizeof(resp), &rlen);
    if (ifdisconnect)
        rpc_disconnect(au->rpc);
#endif /* HWDB_PUBLISH_IN_BACKGROUND */
    return result;
}

#ifdef HWDB_PUBLISH_IN_BACKGROUND
void *do_publish(__attribute__ ((unused)) void *args) {
    CallBackInfo *ca;
    int status;

    for (;;) {
        struct qdecl Q;
        char resp[100];
        unsigned rlen;

        tsuq_take(workQ, (void **)&ca);
        //printf("Removed another call back structure from the workQ\n");
        Q.buf = ca->str;
        Q.size = strlen(Q.buf) + 1;
        status = rpc_call(au_rpc(ca->u.au), &Q, Q.size, resp, sizeof(resp), &rlen);
        free(ca->str);
        ca->str = NULL;
        if (ca->ifdisconnect)
            rpc_disconnect(au_rpc(ca->u.au));
        if (status)	/* successful, free up CallBackInfo struct */
            free(ca);
        else {
            tsuq_add(cleanQ, (void *)ca);  /* free after cleanup */
            //printf("Must clean up registration when back in mainline\n");
        }
    }
    return NULL;
}
#endif /* HWDB_PUBLISH_IN_BACKGROUND */
