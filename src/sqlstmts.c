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

/* sqlstmts.c
 *
 * SQL statement definitions (used by parser)
 *
 * Created by Oliver Sharma on 2009-05-03.
 */
#include "sqlstmts.h"
#include "util.h"
#include "timestamp.h"
#include "gram.h"
#include <stdlib.h>

#include <string.h>

#include "logdefs.h"

const int sql_colattrib_types[6] = {0,1,2,3,4,5};
const char *colattrib_name[6] = {"none", "count", "min", "max", "avg", "sum"};

sqlwindow *sqlstmt_new_stubwindow() {
    sqlwindow *win;

    win = malloc(sizeof(sqlwindow));
    win->type = SQL_WINTYPE_NONE;
    win->num = -1;
    win->unit = -1;

    return win;
}


sqlwindow *sqlstmt_new_timewindow(int num, int unit) {
    sqlwindow *win;

    win = malloc(sizeof(sqlwindow));
    win->type = SQL_WINTYPE_TIME;
    win->num = num;
    win->unit = unit;

    return win;
}

sqlwindow *sqlstmt_new_timewindow_since(char *value) {
    sqlwindow *win;

    win = malloc(sizeof(sqlwindow));
    win->type = SQL_WINTYPE_SINCE;
    win->tstampv = string_to_timestamp(value);

    return win;
}

sqlwindow *sqlstmt_new_timewindow_interval(sqlinterval *val) {
    sqlwindow *win;

    win = malloc(sizeof(sqlwindow));
    win->type = SQL_WINTYPE_INTERVAL;
    (win->intv).leftOp = val->leftOp;
    (win->intv).rightOp = val->rightOp;
    (win->intv).leftTs = val->leftTs;
    (win->intv).rightTs = val->rightTs;

    return win;
}

sqlwindow *sqlstmt_new_timewindow_now() {
    sqlwindow *win;

    win = malloc(sizeof(sqlwindow));
    win->type = SQL_WINTYPE_TIME;
    win->num = -1;
    win->unit = SQL_WINTYPE_TIME_NOW;

    return win;
}


sqlwindow *sqlstmt_new_tuplewindow(int num) {
    sqlwindow *win;

    win = malloc(sizeof(sqlwindow));
    win->type = SQL_WINTYPE_TPL;
    win->num = num;
    win->unit = -1;

    return win;
}

sqlfilter *sqlstmt_new_filter(int ctype, char *name, int dtype, char *value) {
    sqlfilter *filter;

    filter = malloc(sizeof(sqlfilter));
    filter->IS_STR = 0;
    filter->varname = name;
    switch(ctype) {
    case EQUALS:
        filter->sign = SQL_FILTER_EQUAL;
        break;
    case LESS:
        filter->sign = SQL_FILTER_LESS;
        break;
    case GREATER:
        filter->sign = SQL_FILTER_GREATER;
        break;
    case LESSEQ:
        filter->sign = SQL_FILTER_LESSEQ;
        break;
    case GREATEREQ:
        filter->sign = SQL_FILTER_GREATEREQ;
        break;
    case CONTAINS:
        filter->sign = SQL_FILTER_CONTAINS;
        break;
    case NOTCONTAINS:
        filter->sign = SQL_FILTER_NOTCONTAINS;
        break;
    }
    switch(dtype) {
    case INTEGER:
        filter->value.intv = strtoll(value, NULL, 10);
        break;
    case REAL:
        filter->value.realv = strtod(value, NULL);
        break;
    case CHARACTER:
        filter->value.charv = value[0];
        break;
    case TINYINT:
        filter->value.tinyv = atoi(value) & 0xff;
        break;
    case SMALLINT:
        filter->value.smallv = atoi(value) & 0xffff;
        break;
    case TSTAMP:
        filter->value.tstampv = string_to_timestamp(value);
        break;
    case VARCHAR:
        filter->value.stringv = strdup(value);
        filter->IS_STR = 1;
        debugvf("VALUE IS :%s\n",filter->value.stringv);
    }
    return filter;
}

sqlpair *sqlstmt_new_pair(int ctype, char *name, int dtype, char *value) {
    sqlpair *pair;

    pair = malloc(sizeof(sqlpair));
    pair->IS_STR = 0;
    pair->varname = name;
    switch(ctype) {
    case EQUALS:
        pair->sign = SQL_PAIR_EQUAL;
        break;
    case ADD:
        pair->sign = SQL_PAIR_ADDEQ;
        break;
    case SUB:
        pair->sign = SQL_PAIR_SUBEQ;
        break;
    }
    switch(dtype) {
    case INTEGER:
        pair->value.intv = strtoll(value, NULL, 10);
        break;
    case REAL:
        pair->value.realv = strtod(value, NULL);
        break;
    case CHARACTER:
        pair->value.charv = value[0];
        break;
    case TINYINT:
        pair->value.tinyv = atoi(value) & 0xff;
        break;
    case SMALLINT:
        pair->value.smallv = atoi(value) & 0xffff;
        break;
    case TSTAMP:
        pair->value.tstampv = string_to_timestamp(value);
        break;
    case VARCHAR:
        pair->value.stringv = strdup(value);
        pair->IS_STR = 0;
        debugvf("VALUE IS :%s\n",pair->value.stringv);
    }
    return pair;
}

int sqlstmt_calc_len(sqlinsert *insert) {
    int total;
    int i;

    total = strlen(insert->tablename) + 1;
    total += insert->ncols * sizeof(char*);
    for (i=0; i < insert->ncols; i++) {
        total += strlen(insert->colval[i]) + 1;
    }

    debugvf("Calculated insert length to be %d\n", total);
    return total;

}

int sqlstmt_valid_groupby(sqlselect *select) {
    int i, j;
    char *groupbycolname;
    char *selectcolname;
    int found;
    /* if there are none group-by operators, it is correct */
    if (select->groupby_ncols == 0)
        return 1;
    /* if none aggregate operators are specified, then it is incorrect */
    if (!select->containsMinMaxAvgSum && !select->isCountStar) {
        errorf("No aggregate operator specified to group by.\n");
        return 0;
    }
    /* finally, each group-by column must be a selected column */
    for (i = 0; i < select->groupby_ncols; i++) {
        groupbycolname = select->groupby_cols[i];
        found = 0;
        for (j = 0; j < select->ncols; j++) {
            selectcolname = select->cols[j];
            if (strcmp(groupbycolname, selectcolname) == 0)
                found = 1;
        }
        if (!found) {
            errorf("Column %s not selected.\n", groupbycolname);
            return 0;
        }
    }
    return 1;
}

