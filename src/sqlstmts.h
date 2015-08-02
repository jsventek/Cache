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

/* sqlstmts.h
 *
 * SQL statement definitions (used by parser)
 */

#ifndef _SQLSTMTS_H_
#define _SQLSTMTS_H_

#include "typetable.h"

#define SQL_TYPE_SELECT 1
#define SQL_TYPE_CREATE 2
#define SQL_TYPE_INSERT 3
#define SQL_SHOW_TABLES 4
#define SQL_TYPE_UPDATE 5
#define SQL_TYPE_REGISTER 6
#define SQL_TYPE_UNREGISTER 7
#define SQL_TYPE_DELETE 8

#define SQL_WINTYPE_NONE 0
#define SQL_WINTYPE_TIME 1
#define SQL_WINTYPE_TPL 2
#define SQL_WINTYPE_SINCE 3
#define SQL_WINTYPE_INTERVAL 4

#define SQL_WINTYPE_TIME_SECONDS 1
#define SQL_WINTYPE_TIME_MINUTES 2
#define SQL_WINTYPE_TIME_HOURS 3
#define SQL_WINTYPE_TIME_NOW 4
#define SQL_WINTYPE_TIME_MILLIS 5

#define SQL_FILTER_EQUAL 1
#define SQL_FILTER_GREATER 2
#define SQL_FILTER_LESS 3
#define SQL_FILTER_LESSEQ 4
#define SQL_FILTER_GREATEREQ 5
#define SQL_FILTER_CONTAINS 6
#define SQL_FILTER_NOTCONTAINS 7

#define SQL_PAIR_EQUAL 1
#define SQL_PAIR_ADDEQ 2
#define SQL_PAIR_SUBEQ 3

#define SQL_FILTER_TYPE_AND 0
#define SQL_FILTER_TYPE_OR 1

extern const int sql_colattrib_types[];

#define SQL_COLATTRIB_NONE 	&sql_colattrib_types[0]
#define SQL_COLATTRIB_COUNT &sql_colattrib_types[1]
#define SQL_COLATTRIB_MIN 	&sql_colattrib_types[2]
#define SQL_COLATTRIB_MAX 	&sql_colattrib_types[3]
#define SQL_COLATTRIB_AVG 	&sql_colattrib_types[4]
#define SQL_COLATTRIB_SUM 	&sql_colattrib_types[5]

extern const char *colattrib_name[];

union filterval {
    long long intv;
    double realv;
    char charv;
    char tinyv;
    char *stringv;
    short int smallv;
    unsigned long long tstampv;
};

typedef struct sqlinterval {
    int leftOp;
    int rightOp;
    unsigned long long leftTs;
    unsigned long long rightTs;
} sqlinterval;

typedef struct sqlwindow {
    int type;
    int num;			/* used when RANGE */
    int unit;
    unsigned long long tstampv;	/* used when SINCE */
    sqlinterval intv;		/* used when INTERVAL */
} sqlwindow;

typedef struct sqlfilter {
    char *varname;
    int sign; /* =, >, <, <=, >= */
    union filterval value;
    unsigned char IS_STR;
} sqlfilter;

typedef struct sqlselect {
    int ncols;
    char **cols;
    int **colattrib;
    int ntables;
    char **tables;
    sqlwindow **windows;  /* Array of windows (one per table) */
    int nfilters;
    sqlfilter **filters; /* Array of where filters */
    int filtertype; 	/* Temp only. Remove when brackets implemented */
    char *orderby;
    int isCountStar;
    int groupby_ncols;
    char **groupby_cols;
    int containsMinMaxAvgSum;
} sqlselect;

typedef struct sqlpair {
    char *varname;
    int sign; /* =, +=, -= */
    union filterval value;
    unsigned char IS_STR;
} sqlpair;

typedef struct sqlupdate {
    char *tablename;
    int npairs;
    sqlpair **pairs; /* Array of name/operator/value pairs */
    int nfilters;
    sqlfilter **filters; /* Array of where filters */
    int filtertype;
} sqlupdate;

typedef struct sqlcreate {
    char *tablename;
    int ncols;
    char **colname;
    int **coltype;
    short tabletype;
    short primary_column;
} sqlcreate;

typedef struct sqlinsert {
    char *tablename;
    int ncols;
    char **colval;
    int **coltype;
    short transform;
} sqlinsert;

typedef struct sqldelete {
    char* tablename;
    int nfilters;
    sqlfilter **filters;
    int filtertype;
} sqldelete;

typedef struct sqlregister {
    char *automaton;
    char *ipaddr;
    char *port;
    char *service;
} sqlregister;

typedef struct sqlunregister {
    char *id;
} sqlunregister;

typedef struct sqlstmt {
    int type;
    char *name;
    union {
        sqlselect select;
        sqlcreate create;
        sqlinsert insert;
        sqlupdate update;
        sqldelete delete;
        sqlregister regist;
        sqlunregister unregist;
    } sql;
} sqlstmt;

/* Helper functions */

sqlwindow *sqlstmt_new_stubwindow();
sqlwindow *sqlstmt_new_timewindow(int num, int unit);
sqlwindow *sqlstmt_new_timewindow_now();
sqlwindow *sqlstmt_new_timewindow_since(char *value);
sqlwindow *sqlstmt_new_timewindow_interval(sqlinterval *val);
sqlwindow *sqlstmt_new_tuplewindow(int num);

sqlfilter *sqlstmt_new_filter(int ctype, char *name, int dtype, char *value);
sqlfilter *sqlstmt_new_filter_equal(char *name, int value);
sqlfilter *sqlstmt_new_filter_greater(char *name, int value);
sqlfilter *sqlstmt_new_filter_less(char *name, int value);
sqlfilter *sqlstmt_new_filter_greatereq(char *name, int value);
sqlfilter *sqlstmt_new_filter_lesseq(char *name, int value);

sqlpair *sqlstmt_new_pair(int ctype, char *name, int dtype, char *value);

int sqlstmt_calc_len(sqlinsert *insert);
int sqlstmt_valid_groupby(sqlselect *select);

#endif /* _SQLSTMTS_H_ */
