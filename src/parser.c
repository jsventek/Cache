/* parser.c
 *
 * HWDB SQL Parser API
 *
 * Created by Oliver Sharma on 2009-05-04.
 * Copyright (c) 2009. All rights reserved.
 */
#include "parser.h"

#include "sqlstmts.h"
#include "typetable.h"
#include "util.h"
#include "timestamp.h"
#include <stdio.h>
#include <stdlib.h>

extern int yyparse(void);
extern int yyrestart(void);
extern void *yy_scan_string(const char *);
extern void yy_delete_buffer(void*);

//extern int yylex_destroy(void);

extern sqlstmt stmt;

void reset_statement() {
    int i;

    /* NB: possible memory leaks here !!! */

    switch (stmt.type) {

    case SQL_TABLE_META:
        free(stmt.sql.meta.table);
        stmt.type = 0;
        break;

    case SQL_TYPE_REGISTER:
        free(stmt.sql.regist.automaton);
        free(stmt.sql.regist.ipaddr);
        free(stmt.sql.regist.port);
        free(stmt.sql.regist.service);
        stmt.type = 0;
        break;

    case SQL_TYPE_UNREGISTER:
        free(stmt.sql.unregist.id);
        stmt.type = 0;
        break;

    case SQL_TYPE_SELECT:
        if (stmt.sql.select.ncols > 0) {
            for (i = 0; i < stmt.sql.select.ncols; i++)
                free(stmt.sql.select.cols[i]);
            free(stmt.sql.select.cols);
            free(stmt.sql.select.colattrib);
        }
        stmt.sql.select.ncols = 0;
        stmt.sql.select.cols = NULL;
        stmt.sql.select.colattrib = NULL;
        if (stmt.sql.select.ntables > 0) {
            for (i = 0; i < stmt.sql.select.ntables; i++) {
                free(stmt.sql.select.tables[i]);
                free(stmt.sql.select.windows[i]);
            }
            free(stmt.sql.select.tables);
            free(stmt.sql.select.windows);
        }
        stmt.sql.select.ntables = 0;
        stmt.sql.select.tables = NULL;
        stmt.sql.select.windows = NULL;
        if (stmt.sql.select.nfilters > 0) {
            for (i = 0; i < stmt.sql.select.nfilters; i++) {
                free(stmt.sql.select.filters[i]->varname);
                if (stmt.sql.select.filters[i]->IS_STR &&
                        stmt.sql.select.filters[i]->value.stringv) {
                    free(stmt.sql.select.filters[i]->value.stringv);
                }
                free(stmt.sql.select.filters[i]);
            }
            free(stmt.sql.select.filters);
        }
        stmt.sql.select.nfilters = 0;
        stmt.sql.select.filters = NULL;
        stmt.sql.select.filtertype = 0;
        if (stmt.sql.select.groupby_ncols > 0) {
            for (i = 0; i < stmt.sql.select.groupby_ncols; i++)
                free(stmt.sql.select.groupby_cols[i]);
            free(stmt.sql.select.groupby_cols);
        }
        stmt.sql.select.groupby_ncols = 0;
        stmt.sql.select.groupby_cols = NULL;
        if (stmt.sql.select.orderby)
            free(stmt.sql.select.orderby);
        stmt.sql.select.orderby = NULL;
        stmt.sql.select.isCountStar = 0;
        stmt.sql.select.containsMinMaxAvgSum = 0;
        stmt.type = 0;
        break;

    case SQL_TYPE_UPDATE:
        free(stmt.sql.update.tablename);
        if (stmt.sql.update.nfilters > 0) {
            for (i = 0; i < stmt.sql.update.nfilters; i++) {
                free(stmt.sql.update.filters[i]->varname);
                free(stmt.sql.update.filters[i]);
            }
            free(stmt.sql.update.filters);
        }
        stmt.sql.update.nfilters = 0;
        stmt.sql.update.filters = NULL;
        stmt.sql.update.filtertype = 0;
        if (stmt.sql.update.npairs > 0) {
            for (i = 0; i < stmt.sql.update.npairs; i++) {
                free(stmt.sql.update.pairs[i]->varname);
                if (stmt.sql.update.pairs[i]->IS_STR &&
                        stmt.sql.update.pairs[i]->value.stringv) {
                    free(stmt.sql.update.pairs[i]->value.stringv);
                }
                free(stmt.sql.update.pairs[i]);
            }
            free(stmt.sql.update.pairs);
        }
        stmt.sql.update.npairs = 0;
        stmt.sql.update.pairs = NULL;
        stmt.type = 0;
        break;

    case SQL_TYPE_DELETE:
        free(stmt.sql.update.tablename);
        if (stmt.sql.update.nfilters > 0) {
            for (i = 0; i < stmt.sql.update.nfilters; i++) {
                free(stmt.sql.update.filters[i]->varname);
                free(stmt.sql.update.filters[i]);
            }
            free(stmt.sql.update.filters);
        }
        stmt.sql.update.nfilters = 0;
        stmt.sql.update.filters = NULL;
        stmt.sql.update.filtertype = 0;
        stmt.type = 0;
        break;

    case SQL_TYPE_CREATE:
        free(stmt.sql.create.tablename);
        if (stmt.sql.create.ncols > 0) {
            for (i = 0; i < stmt.sql.create.ncols; i++)
                free(stmt.sql.create.colname[i]);
            free(stmt.sql.create.colname);
            free(stmt.sql.create.coltype);
        }
        stmt.sql.create.ncols = 0;
        stmt.sql.create.colname = NULL;
        stmt.sql.create.coltype = NULL;
        stmt.type = 0;
        break;

    case SQL_TYPE_INSERT:
        free(stmt.sql.insert.tablename);
        stmt.sql.insert.tablename = NULL;
        if (stmt.sql.insert.ncols > 0) {
            for (i = 0; i < stmt.sql.insert.ncols; i++) {
                free(stmt.sql.insert.colval[i]);
            }
            free(stmt.sql.insert.colval);
            free(stmt.sql.insert.coltype);
        }
        stmt.sql.insert.ncols = 0;
        stmt.sql.insert.colval = NULL;
        stmt.sql.insert.coltype = NULL;
        stmt.type = 0;
        break;

    case SQL_SHOW_TABLES:
        stmt.type = 0;
        break;

    default:
        stmt.type = 0;
    }

    stmt.type = 0;
}

/* Places parsed output in externally declared global variable:
 *     sqlstmt stmt
 */
void *sql_parse(char *query) {
    int error;
    void *bufstate;

    reset_statement();
    bufstate = yy_scan_string(query);
    error = yyparse();
    sql_reset_parser(bufstate);
    if (error)
        return NULL;
    else
        return bufstate;
}

void sql_reset_parser(void *bufstate) {
    debugvf("resetting parser\n");
    yy_delete_buffer(bufstate);
}

void sql_dup_stmt(sqlstmt *dup) {

    dup->type = stmt.type;

    switch (dup->type) {

    case SQL_TYPE_SELECT:
        dup->sql.select.ncols = stmt.sql.select.ncols;
        dup->sql.select.cols = stmt.sql.select.cols;
        dup->sql.select.ntables = stmt.sql.select.ntables;
        dup->sql.select.tables = stmt.sql.select.tables;
        dup->sql.select.filtertype = stmt.sql.select.filtertype;
        dup->sql.select.orderby = stmt.sql.select.orderby;
        break;

    case SQL_TYPE_CREATE:
        dup->sql.create.tablename = stmt.sql.create.tablename;
        dup->sql.create.ncols = stmt.sql.create.ncols;
        dup->sql.create.colname = stmt.sql.create.colname;
        dup->sql.create.coltype = stmt.sql.create.coltype;
        break;

    case SQL_TYPE_INSERT:
        dup->sql.insert.tablename = stmt.sql.insert.tablename;
        dup->sql.insert.ncols = stmt.sql.insert.ncols;
        dup->sql.insert.colval = stmt.sql.insert.colval;
        dup->sql.insert.coltype = stmt.sql.insert.coltype;
        break;

    default:
        errorf("can't duplicate unknown sqlstmt type\n");

    }
}

/* Prints externally declared global variable
 *     sqlstmt stmt
 * to standard output
 */
void sql_print() {
    int i;

    printf("-----[SQL statement]-------\n");

    switch(stmt.type) {

    case SQL_TYPE_REGISTER:
        printf("Registered %s:%s to automaton\n%s\n",
               stmt.sql.regist.ipaddr, stmt.sql.regist.port, stmt.sql.regist.automaton);
        break;

    case SQL_TYPE_UNREGISTER:
        printf("Unregistered automaton, id = %s\n", stmt.sql.unregist.id);
        break;

    case SQL_TYPE_SELECT:
        printf("Select statement\n");
        if (stmt.sql.select.orderby != NULL) {
            printf("{Ordered by: %s}\n", stmt.sql.select.orderby);
        }
        for (i = 0; i < stmt.sql.select.ncols; i++) {
            printf("col[%d]: %s (colattrib: %s)\n", i, stmt.sql.select.cols[i], colattrib_name[*stmt.sql.select.colattrib[i]]);
        }
        for (i = 0; i < stmt.sql.select.ntables; i++) {
            char *tmpstr;
            int op;
            tstamp_t ts;
            printf("table[%d]: %s ", i, stmt.sql.select.tables[i]);
            switch(stmt.sql.select.windows[i]->type) {
            case SQL_WINTYPE_NONE:
                printf("\n");
                break;

            case SQL_WINTYPE_TIME:
                printf("[time window: %d (unitcode: %d)]\n",
                       stmt.sql.select.windows[i]->num,
                       stmt.sql.select.windows[i]->unit);
                break;

            case SQL_WINTYPE_TPL:
                printf("[tuple window: %d]\n", stmt.sql.select.windows[i]->num);
                break;

            case SQL_WINTYPE_SINCE:
                tmpstr = timestamp_to_string(stmt.sql.select.windows[i]->tstampv);
                printf("[since window: %s]\n", tmpstr);
                free(tmpstr);
                break;

            case SQL_WINTYPE_INTERVAL:
                op = stmt.sql.select.windows[i]->intv.leftOp;
                ts = stmt.sql.select.windows[i]->intv.leftTs;
                tmpstr = timestamp_to_string(ts);
                printf("[interval window: %c%s,", (op == GREATER) ? '(' : '[', tmpstr);
                free(tmpstr);
                op = stmt.sql.select.windows[i]->intv.rightOp;
                ts = stmt.sql.select.windows[i]->intv.rightTs;
                tmpstr = timestamp_to_string(ts);
                printf("%s%c ]\n", tmpstr, (op == LESS) ? ')' : ']');
                free(tmpstr);
                break;

            default:
                errorf("[Unknown window type]\n");
            }
        }
        break;

    case SQL_TYPE_CREATE:
        printf("Create statement\n");
        printf("tablename: %s\n", stmt.sql.create.tablename);
        for (i = 0; i < stmt.sql.create.ncols; i++) {
            printf("name: %s, type %s\n",
                   stmt.sql.create.colname[i],
                   primtype_name[*stmt.sql.create.coltype[i]]);
        }
        break;

    case SQL_TYPE_UPDATE:
        printf("Update statement\n");
        printf("tablename: %s\n", stmt.sql.update.tablename);
        break;

    case SQL_TYPE_INSERT:
        printf("Insert statement\n");
        printf("tablename: %s\n", stmt.sql.insert.tablename);
        for (i = 0; i < stmt.sql.insert.ncols; i++) {
            printf("val: %s, type %s\n",
                   stmt.sql.insert.colval[i],
                   primtype_name[*stmt.sql.insert.coltype[i]]);
        }
        break;

    case SQL_SHOW_TABLES:
        printf("Show tables\n");
        break;

    case SQL_TABLE_META:
        printf("Show table %s\n",stmt.sql.meta.table);
        break;

    default:
        errorf("Unknown statement\n");
    }

    printf("--------------------\n");
}
