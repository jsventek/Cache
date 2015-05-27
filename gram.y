%{

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "util.h"
#include "timestamp.h"
#include "sqlstmts.h"
#include "linkedlist.h"
#include "typetable.h"

extern int yylex();

void yyerror(const char *str) {
    fprintf(stderr,"error: %s\n",str);
}

int yywrap() {
    return 1;
}

/* SQL statement to be returned after parsing */
extern sqlstmt stmt;

/* Temporary lists used while parsing */
/* Select */
LinkedList *clist;
LinkedList *cattriblist;
LinkedList *tlist;
LinkedList *flist;
LinkedList *wlist;
LinkedList *plist; /* pair list */
sqlwindow *tmpwin;
int tmpunit;
sqlfilter *tmpfilter;
sqlpair *tmppair;
int tmpvaltype;
char *tmpvalstr;
int filtertype;
char *orderby;
int countstar;
LinkedList *grouplist;
sqlinterval tmpinterval;
/* Create */
char *tablename;
LinkedList *colnames;
LinkedList *coltypes;
short tabletype;
short primary_column;
short column;
/* Insert */
/* -- tablename definition from above */
/* -- coltypes definition from above */
LinkedList *colvals;
short transform;
/* dummy long for calls to ll_toArray */
static long dummyLong;

%}

%union {
    long long number;
    double numfloat;
    char character;
    char *string;
    unsigned long long tstamp;
}

/*%token <number> NUMBER
%token <numfloat> NUMFLOAT
*/
%token <string> NUMBER
%token <string> NUMFLOAT
%token <string> TSTAMP
%token <string> DATESTRING
%token <string> WORD
%token <string> QUOTEDSTRING
%token <string> IPADDR
/*%token <string> PORT*/

%token SELECT FROM WHERE LESSEQ GREATEREQ LESS GREATER EQUALS COMMA STAR 
%token SEMICOLON CREATE INSERT TABLETK OPENBRKT CLOSEBRKT TABLE INTO VALUES
%token BOOLEAN INTEGER REAL CHARACTER VARCHAR BLOB TINYINT SMALLINT
%token TRUETK FALSETK SINGLEQUOTE PRIMARY KEY
%token OPENSQBRKT CLOSESQBRKT MILLIS SECONDS MINUTES HOURS RANGE
%token SINCE INTERVAL NOW ROWS LAST
%token SHOW TABLES AND OR
%token COUNT MIN MAX AVG SUM
%token ORDER BY
%token REGISTER UNREGISTER
%token PERSISTENTTABLETK
%token GROUP
%token UPDATE SET ADD SUB ON DUPLICATETK
%token CONTAINS NOTCONTAINS

%type <string> tstamp_expr

%%

top:          sqlStmt {
                YYACCEPT;
              }
            | error {
                YYABORT;
              }
            ;
sqlStmt:      selectStmt {
                debugvf("Select statment.\n");
                stmt.type = SQL_TYPE_SELECT;
                /* Columns */
                stmt.sql.select.ncols =  (int)ll_size(clist);
                stmt.sql.select.cols = (char **) ll_toArray(clist, &dummyLong);
                ll_destroy(clist, NULL);
                clist=NULL;
                /* Column attribs (count, min, max, avg, sum) */
                stmt.sql.select.colattrib = (int **) ll_toArray(cattriblist, &dummyLong);
                ll_destroy(cattriblist, NULL);
                cattriblist = NULL;
                /* From Tables */
                stmt.sql.select.ntables = (int)ll_size(tlist);
                stmt.sql.select.tables = (char **) ll_toArray(tlist, &dummyLong);
                ll_destroy(tlist, NULL);
                tlist=NULL;
                /* Table windows */
                if (wlist) {
                  stmt.sql.select.windows = (sqlwindow **) ll_toArray(wlist, &dummyLong);
                  ll_destroy(wlist, NULL);
                  wlist=NULL;
                }
                /* Where filters */
                if (flist) {
                  stmt.sql.select.nfilters = (int)ll_size(flist);
                  stmt.sql.select.filters = (sqlfilter **) ll_toArray(flist, &dummyLong);
                  stmt.sql.select.filtertype = filtertype;
                  ll_destroy(flist, NULL);
                  flist=NULL;
                }
                /* Order by */
                if (orderby) {
                  stmt.sql.select.orderby = orderby;
                } else {
                  stmt.sql.select.orderby = NULL;
                }
                /* Count(*) ? */
                if (countstar) {
                  debugvf("Is count(*)\n");
                  stmt.sql.select.isCountStar = 1;
                } else {
                  debugvf("Not count(*)\n");
                  stmt.sql.select.isCountStar = 0;
                }
                /* Group by */
                if (grouplist) {
                  stmt.sql.select.groupby_ncols =  (int)ll_size(grouplist);
                  stmt.sql.select.groupby_cols = (char **) ll_toArray(grouplist, &dummyLong);
                  ll_destroy(grouplist, NULL);
                  grouplist=NULL;
                } else {
                  stmt.sql.select.groupby_ncols = 0;
                  stmt.sql.select.groupby_cols = NULL;
                }
              }
            | createStmt {
                debugvf("Create statement.\n");
                stmt.type = SQL_TYPE_CREATE;
                stmt.sql.create.tablename = tablename;
                stmt.sql.create.ncols = (int)ll_size(colnames);
                stmt.sql.create.colname = (char **) ll_toArray(colnames, &dummyLong);
                stmt.sql.create.coltype = (int **) ll_toArray(coltypes, &dummyLong);
                ll_destroy(colnames, NULL);
                colnames=NULL;
                ll_destroy(coltypes, NULL);
                coltypes=NULL;
                stmt.sql.create.tabletype = tabletype;
                stmt.sql.create.primary_column = primary_column;
              }
            | insertStmt {
                debugvf("Insert statement.\n");
                stmt.type = SQL_TYPE_INSERT;
                stmt.sql.insert.tablename = tablename;
                stmt.sql.insert.ncols = (int)ll_size(colvals);
                stmt.sql.insert.colval = (char **) ll_toArray(colvals, &dummyLong);
                stmt.sql.insert.coltype = (int **) ll_toArray(coltypes, &dummyLong);
                stmt.sql.insert.transform = transform;
                ll_destroy(colvals, NULL);
                colvals=NULL;
                ll_destroy(coltypes, NULL);
                coltypes=NULL;
              }
            | updateStmt {
                debugvf("Update statement.\n");
                stmt.type = SQL_TYPE_UPDATE;
                stmt.sql.update.tablename = tablename;
                /* Set pairs */
                if (plist) {
                  stmt.sql.update.npairs = (int)ll_size(plist);
                  stmt.sql.update.pairs = (sqlpair **) ll_toArray(plist, &dummyLong);
                  ll_destroy(plist, NULL);
                  plist=NULL;
                }
                /* Where filters */
                if (flist) {
                  stmt.sql.update.nfilters = (int)ll_size(flist);
                  stmt.sql.update.filters = (sqlfilter **) ll_toArray(flist, &dummyLong);
                  stmt.sql.update.filtertype = filtertype;
                  ll_destroy(flist, NULL);
                  flist=NULL;
                }
              }
            | SHOW TABLES {
                debugvf("Show tables.\n");
                stmt.type = SQL_SHOW_TABLES;
              }
            | REGISTER QUOTEDSTRING IPADDR NUMBER WORD {
                debugvf("Register statement: automaton: %s\nip:port:service: %s:%s:%s\n", 
                        (char*)$2,(char*)$3,(char*)$4,(char*)$5);
                stmt.type = SQL_TYPE_REGISTER;
                stmt.sql.regist.automaton = $2;
                stmt.sql.regist.ipaddr = $3;
                stmt.sql.regist.port = $4;
                stmt.sql.regist.service = $5;
              }
            | UNREGISTER NUMBER {
                debugvf("Unregister statement: automaton id: %s\n", (char *)$2);
                stmt.type = SQL_TYPE_UNREGISTER;
                stmt.sql.unregist.id = $2;
              }
            ;

selectStmt:   SELECT all FROM tableList { orderby = NULL;}
            | SELECT all FROM tableList WHERE filterList {orderby = NULL;}
            | SELECT all FROM tableList ORDER BY orderList
            | SELECT all FROM tableList WHERE filterList ORDER BY orderList
            | SELECT colList FROM tableList { orderby = NULL; countstar = 0; }
            | SELECT colList FROM tableList WHERE filterList {
                orderby = NULL; countstar = 0;
              }
            | SELECT colList FROM tableList ORDER BY orderList {countstar = 0;}
            | SELECT colList FROM tableList WHERE filterList ORDER BY orderList {
                countstar = 0;
              }
            | SELECT colList FROM tableList GROUP BY groupList {
                orderby = NULL; countstar = 0;
              }
            | SELECT colList FROM tableList WHERE filterList GROUP BY groupList {
                orderby = NULL; countstar = 0;
              }
            | SELECT colList FROM tableList WHERE filterList GROUP BY groupList ORDER BY orderList {
                countstar = 0;
              }
            ;

colList:      col 
            | colList COMMA col
            ;

col:          WORD {
                debugvf("Col: %s\n", (char *)$1);
                if (!clist)
                  clist = ll_create();
                (void)ll_add(clist, (void *)$1);
                if (!cattriblist)
                  cattriblist = ll_create();
                (void)ll_add(cattriblist, (void *)SQL_COLATTRIB_NONE);
              }
              /*| COUNT OPENBRKT WORD CLOSEBRKT {
                debugvf("Col (COUNT): %s\n", (char *)$3);
                if (!clist)
                  clist = ll_create();
                (void)ll_add(clist, (void *)$3);
                if (!cattriblist)
                  cattriblist = ll_create();
                (void)ll_add(cattriblist, (void *)SQL_COLATTRIB_COUNT);
              } */
            | MIN OPENBRKT WORD CLOSEBRKT {
                debugvf("Col (MIN): %s\n", (char *)$3);
                if (!clist)
                  clist = ll_create();
                (void)ll_add(clist, (void *)$3);
                if (!cattriblist)
                  cattriblist = ll_create();
                (void)ll_add(cattriblist, (void *)SQL_COLATTRIB_MIN);
                stmt.sql.select.containsMinMaxAvgSum = 1;
              }
            | MAX OPENBRKT WORD CLOSEBRKT {
                debugvf("Col (MAX): %s\n", (char *)$3);
                if (!clist)
                  clist = ll_create();
                (void)ll_add(clist, (void *)$3);
                if (!cattriblist)
                  cattriblist = ll_create();
                (void)ll_add(cattriblist, (void *)SQL_COLATTRIB_MAX);
                stmt.sql.select.containsMinMaxAvgSum = 1;
              }
            | AVG OPENBRKT WORD CLOSEBRKT {
                debugvf("Col (AVG): %s\n", (char *)$3);
                if (!clist)
                  clist = ll_create();
                (void)ll_add(clist, (void *)$3);
                if (!cattriblist)
                  cattriblist = ll_create();
                (void)ll_add(cattriblist, (void *)SQL_COLATTRIB_AVG);
                stmt.sql.select.containsMinMaxAvgSum = 1;
              }
            | SUM OPENBRKT WORD CLOSEBRKT {
                debugvf("Col (SUM): %s\n", (char *)$3);
                if (!clist)
                  clist = ll_create();
                (void)ll_add(clist, (void *)$3);
                if (!cattriblist)
                  cattriblist = ll_create();
                (void)ll_add(cattriblist, (void *)SQL_COLATTRIB_SUM);
                stmt.sql.select.containsMinMaxAvgSum = 1;
              }
            ;

all:          STAR {
                debugvf("Select *\n");
                if (!clist)
                  clist = ll_create();
                (void)ll_add(clist, strdup("*"));
                if (!cattriblist)
                  cattriblist = ll_create();
                (void)ll_add(cattriblist, (void *)SQL_COLATTRIB_NONE);
                countstar = 0;
              }
            | COUNT OPENBRKT STAR CLOSEBRKT {
                debugvf("Select count(*)\n");
                if (!clist)
                  clist = ll_create();
                (void)ll_add(clist, strdup("*"));
                if (!cattriblist)
                  cattriblist = ll_create();
                (void)ll_add(cattriblist, (void *)SQL_COLATTRIB_COUNT);
                countstar = 1;
              }
            ;

tableList:    table
            | tableList COMMA table
            ;

table:        WORD {
                debugvf("Table: %s\n", (char *)$1);
                if (!tlist)
                  tlist = ll_create();
                (void)ll_add(tlist, (void *)$1);
                /* Add empty stub window */
                if (!wlist)
                  wlist = ll_create();
                tmpwin = sqlstmt_new_stubwindow();
                (void)ll_add(wlist, (void *)tmpwin);
              }
            | WORD OPENSQBRKT window CLOSESQBRKT {
                debugvf("Table with window: %s\n", (char*)$1);
                if (!tlist)
                  tlist = ll_create();
                (void)ll_add(tlist, (void *)$1);
                /* Add window */
                if (!wlist)
                  wlist = ll_create();
                (void)ll_add(wlist, (void *)tmpwin);
              }
            ;

window:       timewindow
            | tplwindow
            ;

timewindow:   NOW {
                debugvf("TimeWindow NOW\n");
                tmpwin = sqlstmt_new_timewindow_now();
              }
            | RANGE NUMBER unit {
                debugvf("TimeWindow Range %d, unit:%d\n", atoi($2), tmpunit);
                tmpwin = sqlstmt_new_timewindow(atoi($2), tmpunit);
                /* NB: memory leak. need to free $2 */
                free($2);
              }
            | SINCE tstamp_expr {
                debugvf("TimeWindow Since %s\n", $2);
                tmpwin = sqlstmt_new_timewindow_since($2);
                free($2);
              }
            | INTERVAL intvl_expr {
                debugvf("Timewindow Interval\n");
                tmpwin = sqlstmt_new_timewindow_interval(&tmpinterval);
              }
            ;

tstamp_expr:  TSTAMP {
                debugvf("tstamp expression: %s\n", $1);
                $$ = $1;
              }
            | DATESTRING {
                tstamp_t tmp;
                debugvf("datestring expression: %s\n", $1);
                tmp = datestring_to_timestamp($1);
                $$ = timestamp_to_string(tmp);
                free($1);
              }
            ;

intvl_expr:   OPENBRKT tstamp_expr COMMA tstamp_expr CLOSEBRKT {
                tmpinterval.leftOp = GREATER;
                tmpinterval.rightOp = LESS;
                tmpinterval.leftTs = string_to_timestamp($2);
                tmpinterval.rightTs = string_to_timestamp($4);
                free($2);
                free($4);
              }
            | OPENBRKT tstamp_expr COMMA tstamp_expr CLOSESQBRKT {
                tmpinterval.leftOp = GREATER;
                tmpinterval.rightOp = LESSEQ;
                tmpinterval.leftTs = string_to_timestamp($2);
                tmpinterval.rightTs = string_to_timestamp($4);
                free($2);
                free($4);
              }
            | OPENSQBRKT tstamp_expr COMMA tstamp_expr CLOSEBRKT {
                tmpinterval.leftOp = GREATEREQ;
                tmpinterval.rightOp = LESS;
                tmpinterval.leftTs = string_to_timestamp($2);
                tmpinterval.rightTs = string_to_timestamp($4);
                free($2);
                free($4);
              }
            | OPENSQBRKT tstamp_expr COMMA tstamp_expr CLOSESQBRKT {
                tmpinterval.leftOp = GREATEREQ;
                tmpinterval.rightOp = LESSEQ;
                tmpinterval.leftTs = string_to_timestamp($2);
                tmpinterval.rightTs = string_to_timestamp($4);
                free($2);
                free($4);
              }
            ;

unit:         MILLIS {
                debugvf("TimeWindow unit MILLIS\n");
                tmpunit = SQL_WINTYPE_TIME_MILLIS;
              }
            | SECONDS {
                debugvf("TimeWindow unit SECONDS\n");
                tmpunit = SQL_WINTYPE_TIME_SECONDS;
              }
            | MINUTES {
                debugvf("TimeWindow unit MINUTES\n");
                tmpunit = SQL_WINTYPE_TIME_MINUTES;
              }
            | HOURS {
                debugvf("TimeWindow unit HOURS\n");
                tmpunit = SQL_WINTYPE_TIME_HOURS;
              }
            ;

tplwindow:    ROWS NUMBER {
                debugvf("TupleWindow ROWS %d\n", atoi($2));
                tmpwin = sqlstmt_new_tuplewindow(atoi($2));
                /* NB: memory leak. need to free $2 */
                free($2);
              }
            | LAST {
                debugvf("TupleWindow LAST\n");
                tmpwin = sqlstmt_new_tuplewindow(1);
              }
            ;

filterList:   filter
            | filterList AND filter {
                debugvf("Filter type: AND\n");
                filtertype = SQL_FILTER_TYPE_AND;
              }
            | filterList OR filter {
                debugvf("Filter type: OR\n");
                filtertype = SQL_FILTER_TYPE_OR;
              }
            ;

pairList:     pair
            | pairList COMMA pair {
                debugvf("Pair separator: COMMA\n");
              }
            ;

pair:         WORD EQUALS constant {
                debugvf("Pair (WORD=constant): %s = %s\n", (char *)$1, tmpvalstr);
                if (!plist)
                  plist = ll_create();
                tmppair = sqlstmt_new_pair(EQUALS, (char*)$1, tmpvaltype, tmpvalstr);
                (void)ll_add(plist, (void *)tmppair);
                free(tmpvalstr);
              }
            | WORD ADD constant {
                debugvf("Pair (WORD+=constant): %s += %s\n", (char *)$1, tmpvalstr);
                if (!plist)
                  plist = ll_create();
                tmppair = sqlstmt_new_pair(ADD, (char*)$1, tmpvaltype, tmpvalstr);
                (void)ll_add(plist, (void *)tmppair);
                free(tmpvalstr);
              }
            | WORD SUB constant {
                debugvf("Pair (WORD-=constant): %s -= %s\n", (char *)$1, tmpvalstr);
                if (!plist)
                  plist = ll_create();
                tmppair = sqlstmt_new_pair(SUB, (char*)$1, tmpvaltype, tmpvalstr);
                (void)ll_add(plist, (void *)tmppair);
                free(tmpvalstr);
              }
            ;

filter:       WORD EQUALS constant {
                debugvf("Filter (WORD==constant): %s == %s\n",
                        (char *)$1, tmpvalstr);
                if (!flist)
                  flist = ll_create();
                tmpfilter = sqlstmt_new_filter(EQUALS, (char*)$1,
                                               tmpvaltype, tmpvalstr);
                (void)ll_add(flist, (void *)tmpfilter);
                free(tmpvalstr);
              }
            | WORD LESS constant {
                debugvf("Filter (WORD<constant): %s == %s\n",
                        (char *)$1, tmpvalstr);
                if (!flist)
                  flist = ll_create();
                tmpfilter = sqlstmt_new_filter(LESS, (char*)$1,
                                               tmpvaltype, tmpvalstr);
                (void)ll_add(flist, (void *)tmpfilter);
                free(tmpvalstr);
              }
            | WORD GREATER constant {
                debugvf("Filter (WORD>constant): %s == %s\n",
                        (char *)$1, tmpvalstr);
                if (!flist)
                  flist = ll_create();
                tmpfilter = sqlstmt_new_filter(GREATER, (char*)$1,
                                               tmpvaltype, tmpvalstr);
                (void)ll_add(flist, (void *)tmpfilter);
                free(tmpvalstr);
              }
            | WORD LESSEQ constant {
                debugvf("Filter (WORD<=constant): %s == %s\n",
                        (char *)$1, tmpvalstr);
                if (!flist)
                  flist = ll_create();
                tmpfilter = sqlstmt_new_filter(LESSEQ, (char*)$1,
                                               tmpvaltype, tmpvalstr);
                (void)ll_add(flist, (void *)tmpfilter);
                free(tmpvalstr);
              }
            | WORD GREATEREQ constant {
                debugvf("Filter (WORD>=constant): %s == %s\n",
                        (char *)$1, tmpvalstr);
                if (!flist)
                  flist = ll_create();
                tmpfilter = sqlstmt_new_filter(GREATEREQ, (char*)$1,
                                               tmpvaltype, tmpvalstr);
                (void)ll_add(flist, (void *)tmpfilter);
                free(tmpvalstr);
              }
            | WORD CONTAINS constant {
                debugvf("Filter (WORD contains constant): %s contains %s\n",
                        (char *)$1, tmpvalstr);
                if (!flist)
                  flist = ll_create();
                tmpfilter = sqlstmt_new_filter(CONTAINS, (char*)$1, tmpvaltype, tmpvalstr);
                (void)ll_add(flist, (void *)tmpfilter);
                free(tmpvalstr);
              }
            | WORD NOTCONTAINS constant {
                debugvf("Filter (WORD notcontains constant): %s notcontains %s\n",
                        (char *)$1, tmpvalstr);
                if (!flist)
                  flist = ll_create();
                tmpfilter = sqlstmt_new_filter(NOTCONTAINS, (char*)$1, tmpvaltype, tmpvalstr);
                (void)ll_add(flist, (void *)tmpfilter);
                free(tmpvalstr);
              }
            ;

constant:     NUMBER {
                tmpvaltype = INTEGER;
                tmpvalstr = (char *)$1;
              }
            | NUMFLOAT {
                tmpvaltype = REAL;
                tmpvalstr = (char *)$1;
              }
            | tstamp_expr {
                tmpvaltype = TSTAMP;
                tmpvalstr = (char *)$1;
              }
            | QUOTEDSTRING {
                char *p = (char *)malloc(strlen($1));
                sscanf($1,"\"%s\"",p);
       	        int i;
                debugvf("Value varchar: %s\n", $1);
                i = strlen(p) - 1;	/* will point at \" || \n*/
                p[i] = '\0';		/* overwrite it */
                tmpvaltype = VARCHAR;
                tmpvalstr = strdup(p);
              }
            ;

orderList:    WORD {
                debugvf("Order by: %s\n", (char *)$1);
                orderby = $1;
              }
            ;

groupList:    groupcol 
            | groupList COMMA groupcol
            ;

groupcol:     WORD {
                debugvf("Group by col: %s\n", (char *)$1);
                if (!grouplist)
                  grouplist = ll_create();
                (void)ll_add(grouplist, (void *)$1);
              }

createStmt:   CREATE tabDecl WORD { column = 0; } OPENBRKT varDecls CLOSEBRKT {
                debugvf("Tablename: %s\n", (char *)$3);
                tablename = $3;
              }
            ;

tabDecl:      TABLETK {
                debugvf("tabDec: table\n");
                tabletype = 0;
                primary_column = -1;
              }
            | PERSISTENTTABLETK {
                debugvf("tabDec: persistenttable\n");
                tabletype = 1;
                primary_column = -1;
              }
            ;
	
varDecls:     varDec
            | varDecls COMMA varDec
            ;
	
varDec:       WORD BOOLEAN SQLattrib {
                debugvf("varDec boolean: %s\n", $1);
                column++;
                if (!colnames)
                  colnames = ll_create();
                (void)ll_add(colnames, (void *)$1);
                if (!coltypes)
                  coltypes = ll_create();
                (void)ll_add(coltypes, (void *)PRIMTYPE_BOOLEAN);
              }
            | WORD INTEGER SQLattrib {
                debugvf("varDec integer: %s\n", $1);
                column++;
                if (!colnames)
                  colnames = ll_create();
                (void)ll_add(colnames, (void *)$1);
                if (!coltypes)
                  coltypes = ll_create();
                (void)ll_add(coltypes, (void *)PRIMTYPE_INTEGER);
              }
            | WORD REAL SQLattrib {
                debugvf("varDec real: %s\n", $1);
                column++;
                if (!colnames)
                  colnames = ll_create();
                (void)ll_add(colnames, (void *)$1);
                if (!coltypes)
                  coltypes = ll_create();
                (void)ll_add(coltypes, (void *)PRIMTYPE_REAL);
              }
            | WORD CHARACTER SQLattrib {
                debugvf("varDec character: %s\n", $1);
                column++;
                if (!colnames)
                  colnames = ll_create();
                (void)ll_add(colnames, (void *)$1);
                if (!coltypes)
                  coltypes = ll_create();
                (void)ll_add(coltypes, (void *)PRIMTYPE_CHARACTER);
              }
            | WORD VARCHAR OPENBRKT NUMBER CLOSEBRKT SQLattrib {
                debugvf("varDec varchar: %s\n", $1);
                column++;
                if (!colnames)
                  colnames = ll_create();
                (void)ll_add(colnames, (void *)$1);
                if (!coltypes)
                  coltypes = ll_create();
                (void)ll_add(coltypes, (void *)PRIMTYPE_VARCHAR);
                free($4);
              }
            | WORD BLOB OPENBRKT NUMBER CLOSEBRKT SQLattrib {
                debugvf("varDec blob: %s\n", $1);
                column++;
                if (!colnames)
                  colnames = ll_create();
                (void)ll_add(colnames, (void *)$1);
                if (!coltypes)
                  coltypes = ll_create();
                (void)ll_add(coltypes, (void *)PRIMTYPE_BLOB);
              }
            | WORD TINYINT SQLattrib {
                debugvf("varDec tinyint: %s\n", $1);
                column++;
                if (!colnames)
                  colnames = ll_create();
                (void)ll_add(colnames, (void *)$1);
                if (!coltypes)
                  coltypes = ll_create();
                (void)ll_add(coltypes, (void *)PRIMTYPE_TINYINT);
              }
            | WORD SMALLINT SQLattrib {
                debugvf("varDec smallint: %s\n", $1);
                column++;
                if (!colnames)
                  colnames = ll_create();
                (void)ll_add(colnames, (void *)$1);
                if (!coltypes)
                  coltypes = ll_create();
                (void)ll_add(coltypes, (void *)PRIMTYPE_SMALLINT);
              }
            | WORD TSTAMP SQLattrib {
                debugvf("varDec timestamp: %s\n", $1);
                column++;
                if (!colnames)
                  colnames = ll_create();
                (void)ll_add(colnames, (void *)$1);
                if (!coltypes)
                  coltypes = ll_create();
                (void)ll_add(coltypes, (void *)PRIMTYPE_TIMESTAMP);
              }
            ;

SQLattrib:    /* empty */ { /* do nothing */
              }
            | PRIMARY KEY {
                if (! tabletype) {
                  errorf("primary key defined for non-persistent table.\n");
                  YYABORT;
                } else if (primary_column != -1) {
                  errorf("two or more primary keys declared\n");
                  YYABORT;
                } else {
                  primary_column = column;
                }
              }
            ;

insertStmt:   INSERT INTO WORD VALUES OPENBRKT valList CLOSEBRKT {
                debugvf("Tablename: %s\n", (char *)$3);
                tablename = $3;
                transform = 0;
              }
            | INSERT INTO WORD VALUES OPENBRKT valList CLOSEBRKT ON DUPLICATETK KEY UPDATE {
                debugvf("Tablename: %s\n", (char *)$3);
                tablename = $3;
                transform = 1;
              } 
            ;

updateStmt:   UPDATE WORD SET pairList WHERE filterList {
                debugvf("Update table %s\n", (char *)$2);
                tablename = $2;
              }
            ;
	
valList:      val
            | valList COMMA val
            ;

val:          SINGLEQUOTE TRUETK SINGLEQUOTE {
                debugvf("Value bool true\n");
                if (!colvals)
                  colvals = ll_create();
                (void)ll_add(colvals, strdup("1"));
                if (!coltypes)
                  coltypes = ll_create();
                (void)ll_add(coltypes, (void *)PRIMTYPE_BOOLEAN);
              }
            | SINGLEQUOTE FALSETK SINGLEQUOTE {
                debugvf("Value bool false\n");
                if (!colvals)
                  colvals = ll_create();
                (void)ll_add(colvals, strdup("0"));
                if (!coltypes)
                  coltypes = ll_create();
                (void)ll_add(coltypes, (void *)PRIMTYPE_BOOLEAN);
              }
            | SINGLEQUOTE NUMBER SINGLEQUOTE {
                debugvf("Value int: %s\n", $2);
                if (!colvals)
                  colvals = ll_create();
                (void)ll_add(colvals, (void *)$2);
                if (!coltypes)
                  coltypes = ll_create();
                (void)ll_add(coltypes, (void *)PRIMTYPE_INTEGER);
              }
            | SINGLEQUOTE NUMFLOAT SINGLEQUOTE {
                debugvf("Value real: %s\n", $2);
                if (!colvals)
                  colvals = ll_create();
                (void)ll_add(colvals, (void *)$2);
                if (!coltypes)
                  coltypes = ll_create();
                (void)ll_add(coltypes, (void *)PRIMTYPE_REAL);
              }
            | SINGLEQUOTE TSTAMP SINGLEQUOTE {
                debugvf("Value tstamp: %s\n", $2);
                if (!colvals)
                  colvals = ll_create();
                (void)ll_add(colvals, (void *)$2);
                if (!coltypes)
                  coltypes = ll_create();
                (void)ll_add(coltypes, (void *)PRIMTYPE_TIMESTAMP);
              }
            | SINGLEQUOTE WORD SINGLEQUOTE {
                debugvf("Value varchar: %s\n", $2);
                if (!colvals)
                  colvals = ll_create();
                (void)ll_add(colvals, (void *)$2);
                if (!coltypes)
                  coltypes = ll_create();
                (void)ll_add(coltypes, (void *)PRIMTYPE_VARCHAR);
              }
            | QUOTEDSTRING {
                char *p = $1;
                int i;
                debugvf("Value varchar: %s\n", $1);
                i = strlen(p) - 1;	/* will point at \" || \n*/
                p[i] = '\0';		/* overwrite it */
                if (!colvals)
                  colvals = ll_create();
                (void)ll_add(colvals, (void *)strdup(p+1));
                free($1);
                if (!coltypes)
                  coltypes = ll_create();
                (void)ll_add(coltypes, (void *)PRIMTYPE_VARCHAR);
              }
            | error {
                debugvf("Wrong value.\n");
              }
            ;

%%
