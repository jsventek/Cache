/*
 * The Homework Database
 *
 * Results Table
 *
 * Authors:
 *    Oliver Sharma and Joe Sventek
 *     {oliver, joe}@dcs.gla.ac.uk
 *
 * (c) 2009. All rights reserved.
 */
#include "rtab.h"
#include "util.h"
#include "typetable.h"
#include "sqlstmts.h"
#include "adts/linkedlist.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define RTAB_EMPTY "[Empty]"
#define RTAB_EMPTY_LEN 8

static const char *separator = "<|>"; /* separator between packed fields */
static const char *error_msgs[] = {"Success", "Error", "Create_failed",
                                   "Insert_failed", "Save_select_failed",
                                   "Subscribe_failed", "Unsubscribe_failed",
                                   "Parsing failed", "Select_failed",
                                   "Exec_saved_select_failed", "Close_flag",
                                   "Delete_query_failed", "No_tables_defined",
                                   "Update_failed", "Register_failed",
                                   "Unregister_failed"
                                  };

Rtab *rtab_new() {
    Rtab *results;

    results = malloc(sizeof(Rtab));
    results->nrows = 0;
    results->ncols = 0;
    results->colnames = NULL;
    results->coltypes = NULL;
    results->rows = NULL;
    results->mtype = RTAB_MSG_SUCCESS;
    results->msg = (char *)error_msgs[0];

    return results;
}

Rtab *rtab_new_msg(char mtype, char *message) {
    Rtab *results = rtab_new();
    results->mtype = mtype;
    if (! message)
        results->msg = (char *)error_msgs[(int)mtype];
    else
        results->msg = message;
    return results;
}

static void rtab_purge(Rtab *results) {
    int i, j;

    for (i = 0; i < results->ncols; i++)
        free(results->colnames[i]);
    for (i = 0; i < results->nrows; i++) {
        for (j = 0; j < results->ncols; j++)
            free(results->rows[i]->cols[j]);
        free(results->rows[i]->cols);
        free(results->rows[i]);
    }
    free(results->colnames);
    free(results->coltypes);
    free(results->rows);
    results->nrows = 0;
    results->ncols = 0;
    results->colnames = NULL;
    results->coltypes = NULL;
    results->rows = NULL;
}

void rtab_free(Rtab *results) {

    debugvf("Rtab: free.\n");

    if (!results)
        return;

    rtab_purge(results);
    free(results);
}

char **rtab_getrow(Rtab *results, int row) {
    return results->rows[row]->cols;
}

void  rtab_print(Rtab *results) {
    int c, r;
    char **row;

    if (!results) {
        printf("[Results is Empty]\n");
        return;
    }

    printf("==============\nResults:\n");

    printf("Status message: %s\n", results->msg);

    printf("Nrows: %d, Ncols: %d\n", results->nrows, results->ncols);
    if (results->nrows <= 0)
        return;
    printf("Col headers: ");
    for (c=0; c<results->ncols; c++) {
        printf("%s%s (%s)", (c == 0) ? " " : ", ", results->colnames[c],
               primtype_name[*results->coltypes[c]]);
    }
    printf("\n-----------------\n");

    for (r=0; r<results->nrows; r++) {
        row = rtab_getrow(results, r);
        for (c=0; c<results->ncols; c++) {
            printf("%s ", row[c]);
        }
        printf("\n-----------------\n");
    }

    printf("==============\n");
}

/*
 * determine the first row such that packed data from that row to the end
 * fits in a character buffer of "size" bytes
 */
static int start_row(Rtab *results, int size) {
    int buflen, c, r, N;
    char **row;
    int ncols = results->ncols;
    char buf[4096];

    N = 0;
    for (r = results->nrows - 1; r >= 0; r--) {
        row = rtab_getrow(results, r);
        buflen = 0;
        for (c=0; c<ncols; c++) {
            buflen += sprintf(buf+buflen, "%s%s", row[c], separator);
        }
        N += (buflen + 1);
        if (N >= size)
            break;
    }
    return (r+1);		/* went one too far */
}

int rtab_pack(Rtab *results, char *packed, int size, int *len) {
    int sofar, buflen;
    int c, r;
    char **row;
    int ncols = results->ncols;
    int status = 1;
    char buf[4096];

    debugf("Packing rtab\n");

    sofar = 0;

    sofar += sprintf(packed+sofar, "%d%s%s%s", results->mtype, separator, results->msg, separator);
    if (results->nrows <= 0)
        ncols = 0;
    sofar += sprintf(packed+sofar, "%d%s%d%s\n", ncols, separator, results->nrows, separator);
    if (results->nrows <= 0) {
        *len = sofar;
        return status;
    }
    for (c=0; c < results->ncols; c++) {
        sofar += sprintf(packed+sofar, "%s:%s%s", primtype_name[*results->coltypes[c]], results->colnames[c], separator);
    }
    sofar += sprintf(packed+sofar, "\n");
    for (r=start_row(results, size - sofar); r<results->nrows; r++) {
        row = rtab_getrow(results, r);
        buflen = 0;
        for (c=0; c<results->ncols; c++) {
            buflen += sprintf(buf+buflen, "%s%s", row[c], separator);
        }
        buflen += sprintf(buf+buflen, "\n");
        if ((sofar + buflen) > size) {
            status = 0;		/* buffer overrun */
            break;
        }
        sofar += sprintf(packed+sofar, "%s", buf);
    }

    *len = sofar;
    return status;
}

/*
 * routines used by rtab_unpack to obtain integers and strings from
 * the packed buffers received over the network
 */
char *rtab_fetch_int(char *p, int *value) {
    char *q, c;

    if ((q = strstr(p, separator)) != NULL) {
        c = *q;
        *q = '\0';
        *value = atoi(p);
        *q = c;
        q += strlen(separator);
    } else
        *value = 0;
    return q;
}

char *rtab_fetch_str(char *p, char *str, int *len) {
    char *q, c;

    if ((q = strstr(p, separator)) != NULL) {
        c = *q;
        *q = '\0';
        strcpy(str, p);
        *q = c;
        q += strlen(separator);
    } else
        *str = '\0';
    *len = strlen(str);
    return q;
}

int rtab_status(char *packed, char *stsmsg) {
    char *buf;
    int mtype, size;

    buf = packed;
    buf = rtab_fetch_int(buf, &mtype);
    buf = rtab_fetch_str(buf, stsmsg, &size);
    return mtype;
}

Rtab *rtab_unpack(char *packed, int len) {
    Rtab *results;
    char *buf, *p;
    char tmpbuf[1024];
    int mtype, size, ncols, nrows, i, j;

    debugf("Unpacking RTAB\n");
    i = len;			/* eliminate unused warning */
    results = rtab_new();
    buf = packed;
    p = strchr(buf, '\n');
    *p++ = '\0';
    buf = rtab_fetch_int(buf, &mtype);
    buf = rtab_fetch_str(buf, tmpbuf, &size);
    buf = rtab_fetch_int(buf, &ncols);
    buf = rtab_fetch_int(buf, &nrows);
    results->mtype = mtype;
    results->msg = (char *)error_msgs[mtype];
    results->nrows = nrows;
    results->ncols = ncols;
    debugf("RTAB MESSAGE TYPE: %d\n", results->mtype);
    debugf("RTAB MESSAGE: %s\n", results->msg);
    debugf("RTAB NCOLS: %d\n", results->ncols);
    debugf("RTAB NROWS: %d\n", results->nrows);
    if (nrows > 0) {
        results->coltypes = (int **)malloc(ncols * sizeof(int *));
        results->colnames = (char **)malloc(ncols * sizeof(char *));
        buf = p;
        p = strchr(buf, '\n');
        *p++ = '\0';
        for (i = 0; i < ncols; i++) {
            char *cname, *ctype;
            int index;
            buf = rtab_fetch_str(buf, tmpbuf, &size);
            ctype = tmpbuf;
            cname = strchr(tmpbuf, ':');
            *cname++ = '\0';
            index = typetable_index(ctype);
            results->coltypes[i] = &primtype_val[index];
            results->colnames[i] = strdup(cname);
        }
        results->rows = (Rrow **)malloc(nrows * sizeof(Rrow *));
        for (j = 0; j < nrows; j++) {
            Rrow *row;
            buf = p;
            p = strchr(buf, '\n');
            *p++ = '\0';
            row = (Rrow *)malloc(sizeof(Rrow));
            results->rows[j] = row;
            row->cols = (char **)malloc(ncols * sizeof(char *));
            for (i = 0; i < ncols; i++) {
                buf = rtab_fetch_str(buf, tmpbuf, &size);
                row->cols[i] = strdup(tmpbuf);
            }
            if ((p-packed) >= len) {
                results->nrows = j + 1;
                break;
            }
        }
    }

    return results;
}


int rtab_send(Rtab *results, RpcConnection outgoing) {
    Q_Decl(packed,SOCK_RECV_BUF_LEN);
    char resp[100];
    int len;
    unsigned rlen;

    debugf("Sending rtab\n");

    /* Send dummy empty results */
    if (!results) {
        debugvf("Rtab is empty.\n");
        strcpy(packed, RTAB_EMPTY);
        len = RTAB_EMPTY_LEN;
    } else
        (void )rtab_pack(results, packed, SOCK_RECV_BUF_LEN, &len);

    return rpc_call(outgoing, Q_Arg(packed), len, resp, sizeof(resp), &rlen);
}

/* ----------------[ Manipulator methods ] ---------------- */

int cmp_rrow_by_col(const Rrow *ra, const Rrow *rb, int col, int *ct) {

    char **cols_a;
    char **cols_b;

    cols_a = ra->cols;
    cols_b = rb->cols;

    debugvf("cols_a: %p, cols_a[%d]: %s\n", cols_a, col, cols_a[col]);
    debugvf("cols_b: %p, cols_b[%d]: %s\n", cols_b, col, cols_b[col]);

    if (ct == PRIMTYPE_INTEGER) {
        long long i1, i2;
        int ans = 0;
        i1 = strtoll(cols_a[col], NULL, 10);
        i2 = strtoll(cols_b[col], NULL, 10);
        if (i1 < i2)
            ans = -1;
        else if (i1 > i2)
            ans = 1;
        return ans;
    }
    if (ct == PRIMTYPE_TINYINT || ct == PRIMTYPE_SMALLINT) {
        int i1, i2;
        i1 = atoi(cols_a[col]);
        i2 = atoi(cols_b[col]);
        return (i1 - i2);
    }
    if (ct == PRIMTYPE_REAL) {
        double d1, d2;
        int ans = 0;
        d1 = strtod(cols_a[col], NULL);
        d2 = strtod(cols_b[col], NULL);
        if (d1 < d2)
            ans = -1;
        else if (d1 > d2)
            ans = 1;
        return ans;
    }
    return strcmp(cols_a[col], cols_b[col]);
}

/*  quickSort
 *
 *  Variant of public-domain C implementation by Darel Rex Finley.
 *
 *  * This function assumes it is called with valid parameters.
 */

static void quickSort(Rrow **arr, int elements, int col, int *ct) {

#define  MAX_LEVELS  300

    Rrow *piv;
    int  beg[MAX_LEVELS], end[MAX_LEVELS], i=0, L, R, swap ;

    beg[0]=0;
    end[0]=elements;
    while (i>=0) {
        L=beg[i];
        R=end[i]-1;
        if (L<R) {
            piv=arr[L];
            while (L<R) {
                while ((cmp_rrow_by_col(arr[R], piv, col, ct) >= 0) && L<R) R--;
                if (L<R) arr[L++]=arr[R];
                while ((cmp_rrow_by_col(arr[L], piv, col, ct) <= 0) && L<R) L++;
                if (L<R) arr[R--]=arr[L];
            }
            arr[L]=piv;
            beg[i+1]=L+1;
            end[i+1]=end[i];
            end[i++]=L;
            if (end[i]-beg[i]>end[i-1]-beg[i-1]) {
                swap=beg[i];
                beg[i]=beg[i-1];
                beg[i-1]=swap;
                swap=end[i];
                end[i]=end[i-1];
                end[i-1]=swap;
            }
        }
        else {
            i--;
        }
    }
}

void rtab_orderby(Rtab *results, char *colname) {

    int i;
    int valid;
    int *ct;

    if (colname == NULL) {
        debugvf("Rtab: No orderby in select. returning.\n");
        return;
    }

    debugf("Ordering results table by: %s...\n", colname);

    /* Check colname is valid */
    valid = -1;
    for (i = 0; i < results->ncols; i++) {
        if (strcmp(results->colnames[i], colname) == 0) {
            debugf("Order by column is valid, proceeding... (%s)\n", colname);
            valid = i;
            break;
        }
    }
    if (valid == -1) {
        debugf("Order by column is NOT valid\n");
        return;
    }

    /* DEBUG */
    for (i=0; i < results->nrows; i++) {
        debugvf("ptr results->rows[%d]->cols: %p, results->rows[%d]->cols[%d]: %s\n",
                i, results->rows[i]->cols,
                i, valid, results->rows[i]->cols[valid]);
    }
    ct = results->coltypes[valid];
    quickSort(results->rows, results->nrows, valid, ct);
}

void rtab_countstar(Rtab *results) {
    int count;
    //char *countstr;
    char countstr[100];
    char **newcolnames;
    int **newcoltypes;
    Rrow **newrows;
    Rrow *row;

    debugf("rtab_countstar...\n");

    if (results->nrows < 1)
        return;

    //countstr = malloc(sizeof(char)*100);
    //(void) memset((void *)countstr, 0, sizeof(char)*100);

    count = results->nrows;
    sprintf(countstr, "%d", count);

    rtab_purge(results);
    results->nrows = 1;
    results->ncols = 1;

    newcolnames = malloc(sizeof(char*));
    newcolnames[0] = strdup("count(*)");
    results->colnames = newcolnames;

    newcoltypes = malloc(sizeof(int*));
    //newcoltypes[0] = (int *)malloc(sizeof(int));
    newcoltypes[0] = PRIMTYPE_INTEGER;
    results->coltypes = newcoltypes;

    newrows = (Rrow **)malloc(sizeof(Rrow*));
    row = (Rrow *)malloc(sizeof(Rrow));
    newrows[0] = row;
    row->cols = (char **)malloc(sizeof(char *));
    row->cols[0] = strdup(countstr);
    results->rows = newrows;

}

char *rtab_process_min(Rtab *results, int col) {

    int r;
    long long min, ti;
    char **row;
    char tb[100];
    double rmin, tr;
    int *pt;
    int isint;

    debugf("Rtab: process min\n");

    pt = results->coltypes[col];
    if (pt == PRIMTYPE_INTEGER ||
            pt == PRIMTYPE_TINYINT || pt == PRIMTYPE_SMALLINT)
        isint = 1;
    else  if (pt == PRIMTYPE_REAL)
        isint = 0;
    else {
        return strdup("undefined");
    }
    min = 0LL;
    rmin = 0.0;
    if (results->nrows > 0) {

        /* Init value */
        row = rtab_getrow(results, 0);
        if (isint)
            min = strtoll(row[col], NULL, 10);
        else
            rmin = atof(row[col]);

        /* Loop through rest */
        for (r=1; r<results->nrows; r++) {
            row = rtab_getrow(results, r);
            if (isint) {
                ti = strtoll(row[col], NULL, 10);
                if (min > ti)
                    min = ti;
            } else {
                tr = atof(row[col]);
                if (rmin > tr)
                    rmin = tr;
            }
        }
    }
    if (isint)
        sprintf(tb, "%lld", min);
    else
        sprintf(tb, "%f", rmin);
    return strdup(tb);
}

char *rtab_process_max(Rtab *results, int col) {

    int r;
    long long max, ti;
    char **row;
    char tb[100];
    double rmax, tr;
    int *pt;
    int isint;

    debugf("Rtab: process max\n");

    pt = results->coltypes[col];
    if (pt == PRIMTYPE_INTEGER ||
            pt == PRIMTYPE_TINYINT || pt == PRIMTYPE_SMALLINT)
        isint = 1;
    else  if (pt == PRIMTYPE_REAL)
        isint = 0;
    else {
        return strdup("undefined");
    }
    max = 0LL;
    rmax = 0.0;
    if (results->nrows > 0) {

        /* Init value */
        row = rtab_getrow(results, 0);
        if (isint)
            max = strtoll(row[col], NULL, 10);
        else
            rmax = atof(row[col]);

        /* Loop through rest */
        for (r=1; r<results->nrows; r++) {
            row = rtab_getrow(results, r);
            if (isint) {
                ti = strtoll(row[col], NULL, 10);
                if (max < ti)
                    max = ti;
            } else {
                tr = atof(row[col]);
                if (rmax < tr)
                    rmax = tr;
            }
        }
    }
    if (isint)
        sprintf(tb, "%lld", max);
    else
        sprintf(tb, "%f", rmax);
    return strdup(tb);
}

char *rtab_process_avg(Rtab *results, int col) {
    int r;
    char **row;
    unsigned int count;
    double average, x;
    char tb[100];
    int *pt;
    int isint;

    debugf("Rtab: process avg\n");

    pt = results->coltypes[col];
    if (pt == PRIMTYPE_INTEGER ||
            pt == PRIMTYPE_TINYINT || pt == PRIMTYPE_SMALLINT)
        isint = 1;
    else  if (pt == PRIMTYPE_REAL)
        isint = 0;
    else {
        return strdup("undefined");
    }
    average = 0.0;
    if (results->nrows > 0) {

        /* Init value */
        row = rtab_getrow(results, 0);
        count = 1;
        if (isint)
            average = (double)strtoll(row[col], NULL, 10);
        else
            average = atof(row[col]);

        /* Loop through rest */
        for (r=1; r<results->nrows; r++) {
            row = rtab_getrow(results, r);
            if (isint)
                x = (double)strtoll(row[col], NULL, 10);
            else
                x = atof(row[col]);
            average = (x + (double)count * average)/(double)(++count);
        }
    }
    if (isint)
        sprintf(tb, "%lld", (long long)average);
    else
        sprintf(tb, "%f", average);
    return strdup(tb);
}

char *rtab_process_sum(Rtab *results, int col) {

    int r;
    long long sum;
    char **row;
    char tb[100];
    double rsum;
    int *pt;
    int isint;

    debugf("Rtab: process min\n");
    pt = results->coltypes[col];
    if (pt == PRIMTYPE_INTEGER ||
            pt == PRIMTYPE_TINYINT || pt == PRIMTYPE_SMALLINT)
        isint = 1;
    else  if (pt == PRIMTYPE_REAL)
        isint = 0;
    else {
        return strdup("undefined");
    }
    sum = 0LL;
    rsum = 0.0;
    if (results->nrows > 0) {

        /* Loop through the rows */
        for (r=0; r<results->nrows; r++) {
            row = rtab_getrow(results, r);
            if (isint)
                sum += strtoll(row[col], NULL, 10);
            else
                rsum += atof(row[col]);
        }
    }
    if (isint)
        sprintf(tb, "%lld", sum);
    else
        sprintf(tb, "%f", rsum);
    return strdup(tb);

}

void rtab_to_onerow_if_no_others(Rtab *results) {
    int i, j;
    debugf("Rtab: to onerow (if no others)\n");

    /*
     * all selected columns have been min/max/avg/sum'ed
     * therefore, the colnames and coltypes are correct
     * rows[0] has the data, rows[1] ... rows[results->nrows-1] are
     * superfluous, so need to be freed
     */
    for (i = 1; i < results->nrows; i++) {
        for (j = 0; j < results->ncols; j++)
            free(results->rows[i]->cols[j]);
        free(results->rows[i]);
    }
    results->nrows = 1;
}

void rtab_processMinMaxAvgSum(Rtab *results, int** colattrib) {

    int c;
    char *ret;
    int has_non_minMaxAvgSum;

    debugf("Rtab: Processing min, max, avg, sum\n");

    if (results->nrows < 1)
        return;

    has_non_minMaxAvgSum = 0;

    for (c = 0; c < results->ncols; c++) {
        if (*colattrib[c] == *SQL_COLATTRIB_MIN) {
            ret = rtab_process_min(results, c);
            debugf("Min col %d: %s\n", c, ret);
            rtab_replace_col_val(results, c, ret);
            rtab_update_colname(results, c, "min");
        }

        else if (*colattrib[c] == *SQL_COLATTRIB_MAX) {
            ret = rtab_process_max(results, c);
            debugf("Max col %d: %s\n", c, ret);
            rtab_replace_col_val(results, c, ret);
            rtab_update_colname(results, c, "max");
        }

        else if (*colattrib[c] == *SQL_COLATTRIB_AVG) {
            ret = rtab_process_avg(results, c);
            debugf("Avg col %d: %s\n", c, ret);
            rtab_replace_col_val(results, c, ret);
            rtab_update_colname(results, c, "avg");
        }

        else if (*colattrib[c] == *SQL_COLATTRIB_SUM) {
            ret = rtab_process_sum(results, c);
            debugf("Sum col %d: %s\n", c, ret);
            rtab_replace_col_val(results, c, ret);
            rtab_update_colname(results, c, "sum");
        }

        else if (*colattrib[c] == *SQL_COLATTRIB_NONE) {
            has_non_minMaxAvgSum = 1;
        }

    }


    /* Reduce to one row if all of them are minMaxAvgSum */
    if (has_non_minMaxAvgSum == 0) {
        rtab_to_onerow_if_no_others(results);
    }

}

void rtab_replace_col_val(Rtab *results, int c, char *val) {

    int r;
    char **row;

    debugf("Rtab replacing col val\n");

    for (r = 0; r < results->nrows; r++) {
        row = rtab_getrow(results, r);
        free(row[c]);
        row[c] = strdup(val);
    }
    free(val);

}

void rtab_update_colname(Rtab *results, int c, char *prefix) {

    //char *fullname;
    char fullname[100];

    debugf("Rtab: update colname\n");

    //fullname = malloc(sizeof(char)*100); /* TODO: tidy */
    //(void) memset((void *)fullname, 0, 100);
    sprintf(fullname, "%s(%s)", prefix, results->colnames[c]);

    //results->colnames[c] = fullname;
    free(results->colnames[c]);
    results->colnames[c] = strdup(fullname);

}

/* For debugging purposes */
Rtab *rtab_fake_results() {
    Rtab *results;
    int i,j;

    results = rtab_new();

    results->ncols = 3;
    results->nrows = 3;
    results->coltypes = malloc(results->ncols * sizeof(int*));
    results->colnames = malloc(results->ncols * sizeof(char*));
    results->rows = malloc(results->nrows * sizeof(Rrow*));
    for (i=0; i<results->nrows; i++) {
        results->rows[i] = malloc(sizeof(Rrow));
        results->rows[i]->cols = malloc(results->ncols * sizeof(char*));
    }

    for (i=0; i < results->ncols; i++) {
        results->coltypes[i] = PRIMTYPE_VARCHAR;
        results->colnames[i] = "Whatever";
        for (j=0; j < results->nrows; j++) {
            results->rows[j]->cols[i] = "Hmm";
        }
    }

    return results;
}

static void rtab_update_colnames(Rtab *results, Rtab* newresults) {
    int c;
    for (c = 0; c < results->ncols; c++) {
        free(results->colnames[c]);
        results->colnames[c] = strdup(newresults->colnames[c]);
    }
    return;
}

/* The group-by operator */

typedef struct group_t *groupP;
typedef struct group_t {
    groupP next;
    Rtab *t; /* */
    LinkedList *rowlist;
    char *key;
} group_t;
#define NKEYS 100
#define MULT 31
static groupP bin[NKEYS];
static int groupsize = 0;
static int gverbose = 0;

static void groupby_free() {
    unsigned int i;
    groupP p, q;
    for (i = 0; i < NKEYS; i++) {
        for (p = bin[i]; p != NULL; p = q) {
            q = p->next;
            free(p->key);
            rtab_free(p->t);
            ll_destroy(p->rowlist, NULL);
            free(p);
        }
    }
}

static void groupby_dump() {
    unsigned int i;
    groupP p;
    debugf("Dump table of size %d\n", groupsize);
    for (i = 0; i < NKEYS; i++) {
        for (p = bin[i]; p != NULL; p = p->next) {
            debugf("%s ->\n", p->key);
            rtab_print(p->t);
        }
    }
}

static void groupby_init() {
    unsigned int i;
    for (i = 0; i < NKEYS; i++)
        bin[i] = NULL;
    groupsize = 0;
}

static unsigned int groupby_hash(char *key) {
    unsigned int h = 0;
    for (; *key; key++)
        h = MULT * h + *key;
    return h % NKEYS;
}

static groupP groupby_get(char *key) {
    groupP p;
    unsigned int h = groupby_hash(key);
    for (p = bin[h]; p != NULL; p = p->next) {
        if (strcmp(key, p->key) == 0) {
            return p;
        }
    }
    return NULL;
}

static groupP groupby_set(char *key, Rtab* results) {
    int i;
    groupP p;
    unsigned int h = groupby_hash(key);
    p = malloc(sizeof(group_t));
    p->key = strdup(key);
    p->t = rtab_new();
    /* create new rtab with the same columns as `results` */
    p->t->ncols = results->ncols;
    p->t->colnames = (char **)malloc(results->ncols * sizeof(char *));
    p->t->coltypes = malloc(results->ncols * sizeof(int*));
    p->t->ncols = results->ncols;
    for (i = 0; i < results->ncols; i++) {
        p->t->colnames[i] = strdup(results->colnames[i]);
        p->t->coltypes[i] = results->coltypes[i];
    }
    p->t->nrows = 0;
    /* create a row list that holds the relevant rows */
    p->rowlist = ll_create();
    /* add to hashtable */
    groupsize++;
    p->next = bin[h];
    bin[h] = p;
    return p;
}

static void groupby_addrow(groupP p, Rrow *row) {
    int i;
    Rrow *r;
    r = malloc(sizeof(Rrow));
    r->cols = malloc(p->t->ncols * sizeof(char*));
    for (i = 0; i < p->t->ncols; i++) {
        r->cols[i] = strdup(row->cols[i]);
        debugf("copied value is %s\n", r->cols[i]);
    }
    (void)ll_add(p->rowlist, r);
    return;
}

static void groupby_explode() {
    int i;
    groupP p;
    for (i = 0; i < NKEYS; i++) {
        for (p = bin[i]; p != NULL; p = p->next) {
            long dummyLen;
            /* Build array from rowlist */
            p->t->nrows = ll_size(p->rowlist);
            p->t->rows = (Rrow**) ll_toArray(p->rowlist, &dummyLen);
        }
    }
    return;
}

static void groupby_implode(Rtab *results, int isCountStar,
                            int containsMinMaxAvg, int **colattrib) {
    int i, j;
    groupP p;
    LinkedList* newrowlist;
    long dummyLen;
    /* clear results */
    for (i = 0; i < results->nrows; i++) {
        for (j = 0; j < results->ncols; j++) {
            free(results->rows[i]->cols[j]);
        }
        free(results->rows[i]->cols);
        free(results->rows[i]);
    }
    /* repopulate results */
    newrowlist = ll_create();
    for (i = 0; i < NKEYS; i++) {
        for (p = bin[i]; p != NULL; p = p->next) {
            if (isCountStar) {
                debugf("not yet implemented");
            } else if (containsMinMaxAvg) {
                rtab_processMinMaxAvgSum(p->t, colattrib);
                rtab_update_colnames(results, p->t);
            }
            /* only the last row is of interest for the results */
            Rrow *row = p->t->rows[p->t->nrows-1];
            Rrow *r;
            r = malloc(sizeof(Rrow));
            r->cols = malloc(p->t->ncols * sizeof(char*));
            for (j = 0; j < p->t->ncols; j++) {
                r->cols[j] = strdup(row->cols[j]);
                debugf("copied value is %s\n", r->cols[j]);
            }
            (void)ll_add(newrowlist, r);
        }
    }
    /* rowlist contains all the relevant rows; append to results */
    results->nrows = ll_size(newrowlist);
    results->rows = (Rrow**) ll_toArray(newrowlist, &dummyLen);
    ll_destroy(newrowlist, NULL);
    return;
}

void rtab_groupby(Rtab *results, int ncols, char** cols,
                  int isCountStar, int containsMinMaxAvg, int** colattrib) {
    int i, j, r;
    char **row;
    char key[1024];
    groupP p;
    int *index = (int *) malloc(ncols * sizeof(int));
    groupby_init();
    debugf("Rtab: grouping by\n");
    for (i = 0; i < ncols; i++) {
        debugf("%s\n", cols[i]);
        for (j = 0; j < results->ncols; j++) {
            if (strcmp(results->colnames[j], cols[i]) == 0) {
                debugf("index is %d\n", j);
                *(index + i) = j;
            }
        }
    }
    if (gverbose > 0) {
        debugf("dumping indices\n");
        for (i = 0; i < ncols; i++) {
            debugf("%d = %i\n", i, *(index + i));
        }
    }
    /* iterate through results */
    for (r = 0; r < results->nrows; r++) {
        row = rtab_getrow(results, r);
        /* memset key to 0 */
        memset(key, 0, 1024);
        /* construct key */
        j = 0;
        for (i = 0; i < ncols; i++) {
            j += sprintf(key+j, "%s%s", row[*(index+i)], separator);
        }
        debugf("row %d's key is %s\n", r, key);
        /* lookup in hashtable */
        if ((p = groupby_get(key)) == NULL) {
            debugf("Key does not exist. Insert.\n");
            p = groupby_set(key, results);
        } else {
            debugf("Key exists.\n");
        }
        groupby_addrow(p, results->rows[r]);
    }
    groupby_explode();
    groupby_implode(results, isCountStar, containsMinMaxAvg, colattrib);
    if (gverbose > 0) groupby_dump();
    free(index);
    groupby_free();
    return;
}

