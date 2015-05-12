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

#ifndef _RTAB_H_
#define _RTAB_H_

#include "config.h"
#include "srpc.h"

/* Rtab Status flags */
#define RTAB_MSG_SUCCESS 0
#define RTAB_MSG_ERROR 1
#define RTAB_MSG_CREATE_FAILED 2
#define RTAB_MSG_INSERT_FAILED 3
#define RTAB_MSG_SAVE_SELECT_FAILED 4
#define RTAB_MSG_SUBSCRIBE_FAILED 5
#define RTAB_MSG_UNSUBSCRIBE_FAILED 6
#define RTAB_MSG_PARSING_FAILED 7
#define RTAB_MSG_SELECT_FAILED 8
#define RTAB_MSG_EXEC_SAVED_SELECT_FAILED 9
#define RTAB_MSG_CLOSE_FLAG 10
#define RTAB_MSG_DELETE_QUERY_FAILED 11
#define RTAB_MSG_NO_TABLES_DEFINED 12
#define RTAB_MSG_UPDATE_FAILED 13
#define RTAB_MSG_REGISTER_FAILED 14
#define RTAB_MSG_UNREGISTER_FAILED 15

typedef struct rrow {
    char **cols;		/* All data stored as strings */
} Rrow;

typedef struct rtab {
    int nrows;
    int ncols;
    char **colnames;		/* Array of column names */
    int  **coltypes;		/* Array of column data types */
    Rrow **rows; 		/* Array of rows (data) */
    /* Embedded messages (e.g. error messages) */
    char mtype;
    char *msg;
} Rtab;

Rtab *rtab_new();
Rtab *rtab_new_msg(char mtype, char *message);
void rtab_free(Rtab *results);
char **rtab_getrow(Rtab *results, int row);
void rtab_print(Rtab *results);
int rtab_pack(Rtab *results, char *packed, int size, int *len);
Rtab *rtab_unpack(char *packed, int len);
int rtab_status(char *packed, char *stsmsg);
int rtab_send(Rtab *results, RpcConnection outgoing);

/* Manipulators */
void rtab_orderby(Rtab *results, char *colname);
void rtab_groupby(Rtab *results, int ncols, char** cols,
                  int isCountStar, int containsMinMaxAvg, int** colattrib);
void rtab_countstar(Rtab *results);
char *rtab_process_min(Rtab *results, int col);
char *rtab_process_max(Rtab *results, int col);
char *rtab_process_avg(Rtab *results, int col);
char *rtab_process_sum(Rtab *results, int col);
void rtab_to_onerow_if_no_others(Rtab *results);
void rtab_processMinMaxAvgSum(Rtab *results, int** colattrib);
void rtab_replace_col_val(Rtab *results, int c, char *val);
void rtab_update_colname(Rtab *results, int c, char *prefix);

/* Utility functions */
char *rtab_fetch_int(char *p, int *value);
char *rtab_fetch_str(char *p, char *str, int *len);

/* for debugging */
Rtab *rtab_fake_results();

#endif /* _RTAB_H_ */
