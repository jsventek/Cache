%{
/*
 * Copyright (c) 2013, Court of the University of Glasgow
 * Copyright (c) 2018, University of Oregon
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

#include "adts/linkedlist.h"
#include "adts/hashmap.h"
#include "adts/arraylist.h"
#include "code.h"
#include "dataStackEntry.h"
#include "machineContext.h"
#include "timestamp.h"
#include "event.h"
#include "topic.h"
#include "a_globals.h"
#include "dsemem.h"
#include "extreg.h"
#include "ptable.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <setjmp.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#define TRUE  1
#define FALSE 0

int a_parse();
int a_lex();
int a_error();
void a_init();
int backslash();
int follow();
void field_split();
static DataStackEntry dse;
static LinkedList *vblnames = NULL;
static HashMap *vars2strs = NULL;
static int lineno = 1;
static char *infile;		/* input file name */
static char autoprog[10240];	/* holds text of automaton */
static char *ap;		/* pointer used by get_ch and unget_ch */
static char **gargv;		/* global argument list */
static int gargc;		/* global argument count */
HashMap *vars2index = NULL;
HashMap *topics = NULL;
HashMap *builtins = NULL;
HashMap *devconstants = NULL;
ArrayList *variables;
ArrayList *index2vars;
char *progname;
char errbuf[1024];
%}

%union {
    char *strv;
    double dblv;
    long long intv;
    unsigned long long tstampv;
    InstructionEntry *inst;
}
%token	<strv>	VAR FIELD STRING FUNCTION PROCEDURE PARAMTYPE /* tokens that malloc */
%token	<intv>	SUBSCRIBE TO WHILE IF ELSE INITIALIZATION BEHAVIOR MAP PRINT
%token	<intv>	BOOLEAN INTEGER ROWS SECS WINDOW DESTROY NULLVAL
%token	<intv>	BOOLDCL INTDCL REALDCL STRINGDCL TSTAMPDCL IDENTDCL SEQDCL EVENTDCL
%token  <intv>  ITERDCL MAPDCL WINDOWDCL
%token  <intv>  ASSOCIATE WITH PLUSEQ MINUSEQ
%token	<dblv>	DOUBLE
%token	<tstampv> TSTAMP
%type	<strv>	variable
%type	<intv>	variabletype basictype constructedtype maptype windowtype parameterizedtype
%type	<intv>	argumentlist winconstr
%type	<inst>	condition while end expr begin if else
%type	<inst>	statement assignment pluseq minuseq statementlist body
%right	'='
%left	OR
%left	AND
%left	'|'
%left	'&'
%left	GT GE LT LE EQ NE
%left	'+' '-'
%left	'*' '/' '%'
%left	UNARYMINUS NOT
%right	'^'

%%

automaton:        subscriptions behavior {
                    code(TRUE, STOP, NULL, "STOP", lineno);
                    YYACCEPT;
                  }
                | subscriptions declarations behavior {
                    code(TRUE, STOP, NULL, "STOP", lineno);
                    YYACCEPT;
                  }
                | subscriptions declarations initialization behavior {
                    code(TRUE, STOP, NULL, "STOP", lineno);
                    YYACCEPT;
                  }
                | subscriptions associations behavior {
                    code(TRUE, STOP, NULL, "STOP", lineno);
                    YYACCEPT;
                  }
                | subscriptions associations declarations behavior {
                    code(TRUE, STOP, NULL, "STOP", lineno);
                    YYACCEPT;
                  }
                | subscriptions associations declarations initialization behavior {
                    code(TRUE, STOP, NULL, "STOP", lineno);
                    YYACCEPT;
                  }
                | error {
                    YYABORT;
                  }
                ;
subscriptions:	  subscription
                | subscriptions subscription
                ;
subscription:	  SUBSCRIBE VAR TO VAR ';' {
                    void *dummy;
                    long index;
                    if (! top_exist($4)) {
                      comperror($4, ": non-existent topic");
                      YYABORT;
                    }
                    if (hm_containsKey(vars2index, $2)) {
                      comperror($2, ": variable already defined");
                      YYABORT;
                    }
                    index = al_size(variables);
                    (void) hm_put(vars2index, $2, (void *)index, &dummy);
                    (void) hm_put(topics, $4, (void *)index, &dummy);
                    (void) hm_put(vars2strs, $2, $4, &dummy);
                    initDSE(&dse, dEVENT, NOTASSIGN);
                    dse.value.ev_v = NULL;
                    (void) al_insert(variables, index, dse_duplicate(dse));
                    (void) al_insert(index2vars, index, $2);
                  }
                ;
associations:     association
                | associations association
                ;
association:      ASSOCIATE VAR WITH VAR ';' {
                    void *dummy;
                    long index;
                    if (! ptab_exist($4)) {
                      comperror($4, ": non-existent persistent table");
                      YYABORT;
                    }
                    if (hm_containsKey(vars2index, $2)) {
                      comperror($2, ": variable already defined");
                      YYABORT;
                    }
                    initDSE(&dse, dPTABLE, NOTASSIGN);
                    dse.value.str_v = $4;
                    index = al_size(variables);
                    (void) hm_put(vars2index, $2, (void *)index, &dummy);
                    (void) al_insert(variables, index, dse_duplicate(dse));
                    (void) al_insert(index2vars, index, $2);
                  }
                ;
declarations:	  declaration
                | declarations declaration
                ;
declaration:	  variabletype variablelist ';' {
                    char *p;
                    void *dummy;
                    long index;
                    initDSE(&dse, $1, 0);
                    while (ll_removeFirst(vblnames, (void **)&p)) {
                      if (hm_containsKey(vars2index, p)) {
                        comperror(p, ": variable previously defined");
                        YYABORT;
                      }
                      switch(dse.type) {
                      case dBOOLEAN:	dse.value.bool_v = 0; break;
                      case dINTEGER:	dse.value.int_v = 0; break;
                      case dDOUBLE:	dse.value.dbl_v = 0; break;
                      case dTSTAMP:	dse.value.tstamp_v = 0; break;
                      case dSTRING:	dse.value.str_v = NULL; break;
                      case dMAP:	dse.value.map_v = NULL; break;
                      case dIDENT:	dse.value.str_v = NULL; break;
                      case dWINDOW:	dse.value.win_v = NULL; break;
                      case dITERATOR:	dse.value.iter_v = NULL; break;
                      case dSEQUENCE:	dse.value.seq_v = NULL; break;
                      }
                      index = al_size(variables);
                      (void) hm_put(vars2index, p, (void *)index, &dummy);
                      (void) al_insert(variables, index, dse_duplicate(dse));
                      (void) al_insert(index2vars, index, p);
                    }
                    ll_destroy(vblnames, NULL); vblnames = NULL;
                  }
                | parameterizedtype PARAMTYPE variablelist ';' {
                    char *p;
                    void *dummy;
                    long index;
                    initDSE(&dse, $1, 0);
                    while (ll_removeFirst(vblnames, (void **)&p)) {
                      if (hm_containsKey(vars2index, p)) {
                        comperror(p, ": variable previously defined");
                        YYABORT;
                      }
                      dse.value.ev_v = NULL;
                      index = al_size(variables);
                      (void) hm_put(vars2index, p, (void *)index, &dummy);
                      (void) hm_put(vars2strs, p, strdup($2), &dummy);
                      (void) al_insert(variables, index, dse_duplicate(dse));
                      (void) al_insert(index2vars, index, p);
                    }
                    free($2);
                    ll_destroy(vblnames, NULL);
                    vblnames = NULL;
                  }
                ;
basictype:	  INTDCL    { $$ = dINTEGER; }
                | BOOLDCL   { $$ = dBOOLEAN; }
                | REALDCL   { $$ = dDOUBLE; }
                | STRINGDCL { $$ = dSTRING; }
                | TSTAMPDCL { $$ = dTSTAMP; }
                ;
constructedtype:  SEQDCL    { $$ = dSEQUENCE; }
                | IDENTDCL  { $$ = dIDENT; }
                | ITERDCL   { $$ = dITERATOR; }
                | MAPDCL    { $$ = dMAP; }
                | WINDOWDCL { $$ = dWINDOW; }
                | EVENTDCL  { $$ = dEVENT; }
                ;
parameterizedtype: EVENTDCL { $$ = dEVENT; }
                ;
variabletype:     basictype
                | constructedtype
                ;
maptype:          basictype
                | SEQDCL    { $$ = dSEQUENCE; }
                | WINDOWDCL { $$ = dWINDOW; }
                ;
windowtype:       basictype
                | SEQDCL	{ $$ = dSEQUENCE; }
variablelist:	  variable
                | variablelist ',' variable
                ;
variable:	  VAR {
                    if (! vblnames)
                      vblnames = ll_create();
                    ll_addLast(vblnames, (void *)$1);
                  }
                ;
winconstr:        ROWS { $$ = dROWS; }
                | SECS { $$ = dSECS; }
                ;
initialization:	  INITIALIZATION '{' statementlist '}' {
                    code(TRUE, STOP, NULL, "STOP", lineno);
                  }
                ;
behavior:	  BEHAVIOR { switchcode(); } '{' statementlist '}' {
                    code(TRUE, STOP, NULL, "STOP", lineno);
                    endcode();
                  }
                ;
statementlist:	  statement
                | statementlist statement
                ;
statement:	  ';' {
                    $$ = (InstructionEntry *)0;
                  }
                | expr ';'
                | PRINT '(' expr ')' ';' {
                     code(TRUE, print, NULL, "print", lineno);
                     $$ = $3;
                  }
                | DESTROY '(' VAR ')' ';' {
                    void *value;
                    if (! hm_containsKey(vars2index, $3)) {
                      comperror($3, ": undefined variable");
                      YYABORT;
                    }
                    code(TRUE, destroy, NULL, "destroy", lineno);
                    initDSE(&dse, dINTEGER, 0);
                    (void) hm_get(vars2index, $3, &value);
                    dse.value.int_v = (long long)value;
                    code(FALSE, STOP, &dse, "variable name", lineno);
                    free($3);
                  }
                | PROCEDURE '(' argumentlist ')' ';' {
                    if (iflog)
                      fprintf(stderr, "%s called, #args = %lld\n", $1, $3);
                    code(TRUE, procedure, NULL, "procedure", lineno);
                    initDSE(&dse, dSTRING, 0);
                    dse.value.str_v = $1;
                    code(FALSE, STOP, &dse, "procname", lineno);
                    initDSE(&dse, dINTEGER, 0);
                    dse.value.int_v = $3;
                    code(FALSE, STOP, &dse, "nargs", lineno);
                  }
                | while condition begin body end {
                    ($1)[1].type = PNTR;
                    ($1)[1].u.offset = $3 - $1;
                    ($1)[2].type = PNTR;
                    ($1)[2].u.offset = $5 - $1;
                  }
                | if condition begin body end {
                    ($1)[1].type = PNTR;
                    ($1)[1].u.offset = $3 - $1;
                    ($1)[2].type = PNTR;
                    ($1)[2].u.offset = 0;
                    ($1)[3].type = PNTR;
                    ($1)[3].u.offset = $5 - $1;
                  }
                | if condition begin body else begin body end {
                    ($1)[1].type = PNTR;
                    ($1)[1].u.offset = $3 - $1;
                    ($1)[2].type = PNTR;
                    ($1)[2].u.offset = $6 - $1;
                    ($1)[3].type = PNTR;
                    ($1)[3].u.offset = $8 - $1;
                  }
                | '{' statementlist '}' {
                    $$ = $2;
                  }
                ;
body:             statement {
                    if (iflog)
                      fprintf(stderr, "Starting code generation of body\n");
                  }
                ;
argumentlist:	  /* empty */			{ $$ = 0; }
                | expr				{ $$ = 1; }
                | argumentlist ',' expr		{ $$ = $1 + 1; }
                ;
assignment:	  VAR '=' expr {
                    void *value;
                    if (! hm_containsKey(vars2index, $1)) {
                      comperror($1, ": undefined variable");
                      YYABORT;
                    }
                    code(TRUE, varpush, NULL, "varpush", lineno);
                    initDSE(&dse, dINTEGER, 0);
                    (void) hm_get(vars2index, $1, &value);
                    dse.value.int_v = (long long)value;
                    code(FALSE, STOP, &dse, "variable name", lineno);
                    code(TRUE, assign, NULL, "assign", lineno);
                    free($1);
                    $$ = $3;
                  }
                ;
pluseq:	          VAR PLUSEQ expr {
                    void *value;
                    if (! hm_containsKey(vars2index, $1)) {
                      comperror($1, ": undefined variable");
                      YYABORT;
                    }
                    code(TRUE, varpush, NULL, "varpush", lineno);
                    (void) hm_get(vars2index, $1, &value);
                    initDSE(&dse, dINTEGER, 0);
                    dse.value.int_v = (long long)value;
                    code(FALSE, STOP, &dse, "variable name", lineno);
                    code(TRUE, pluseq, NULL, "pluseq", lineno);
                    free($1);
                    $$ = $3;
                  }
                ;
minuseq:          VAR MINUSEQ expr {
                    void *value;
                    if (! hm_containsKey(vars2index, $1)) {
                      comperror($1, ": undefined variable");
                      YYABORT;
                    }
                    code(TRUE, varpush, NULL, "varpush", lineno);
                    (void) hm_get(vars2index, $1, &value);
                    initDSE(&dse, dINTEGER, 0);
                    dse.value.int_v = (long long)value;
                    code(FALSE, STOP, &dse, "variable name", lineno);
                    code(TRUE, minuseq, NULL, "minuseq", lineno);
                    free($1);
                    $$ = $3;
                  }
                ;
condition:	  '(' expr ')' {
                    code(TRUE, STOP, NULL, "end condition", lineno);
                    $$ = $2;
                  }
                ;
while:		  WHILE {
                    InstructionEntry *spc;
                    if (iflog)
                      fprintf(stderr, "Starting code generation for while\n");
                    spc = code(TRUE, whilecode, NULL, "whilecode", lineno);
                    code(TRUE, STOP, NULL, "whilebody", lineno);
                    code(TRUE, STOP, NULL, "nextstatement", lineno);
                    $$ = spc;
                  }
                ;
if:		  IF {
                    InstructionEntry *spc;
                    if (iflog)
                      fprintf(stderr, "Starting code generation for if\n");
                    spc = code(TRUE, ifcode, NULL, "ifcode", lineno);
                    code(TRUE, STOP, NULL, "thenpart", lineno);
                    code(TRUE, STOP, NULL, "elsepart", lineno);
                    code(TRUE, STOP, NULL, "nextstatement", lineno);
                    $$ = spc;
                  }
                ;
else:		  ELSE {
                    code(TRUE, STOP, NULL, "STOP", lineno); $$ = progp;
                  }
                ;
begin:		  /* nothing */ { $$ = progp; }
                ;
end:		  /* nothing */ {
                    code(TRUE, STOP, NULL, "STOP", lineno); $$ = progp;
                  }
                ;
expr:             NULLVAL {
                    code(TRUE, constpush, NULL, "constpush", lineno);
                    initDSE(&dse, dNULL, 0);
                    $$ = code(FALSE, NULL, &dse, "NULL", lineno);
                  }
                | INTEGER {
                    code(TRUE, constpush, NULL, "constpush", lineno);
                    initDSE(&dse, dINTEGER, 0);
                    dse.value.int_v = $1;
                    $$ = code(FALSE, NULL, &dse, "integer literal", lineno);
                  }
                | DOUBLE {
                    code(TRUE, constpush, NULL, "constpush", lineno);
                    initDSE(&dse, dDOUBLE, 0);
                    dse.value.dbl_v = $1;
                    $$ = code(FALSE, NULL, &dse, "real literal", lineno);
                  }
                | BOOLEAN {
                    code(TRUE, constpush, NULL, "constpush", lineno);
                    initDSE(&dse, dBOOLEAN, 0);
                    dse.value.bool_v = $1;
                    $$ = code(FALSE, NULL, &dse, "boolean literal", lineno);
                  }
                | TSTAMP {
                    code(TRUE, constpush, NULL, "constpush", lineno);
                    initDSE(&dse, dTSTAMP, 0);
                    dse.value.tstamp_v = $1;
                    $$ = code(FALSE, NULL, &dse, "timestamp literal", lineno);
                  }
                | STRING {
                    code(TRUE, constpush, NULL, "constpush", lineno);
                    initDSE(&dse, dSTRING, 0);
                    dse.value.str_v = $1;
                    $$ = code(FALSE, NULL, &dse, "string literal", lineno);
                  }
                | VAR {
                    InstructionEntry *spc;
                    void *value;
                    if (! hm_containsKey(vars2index, $1)) {
                      comperror($1, ": undefined variable");
                      YYABORT;
                    }
                    spc = code(TRUE, varpush, NULL, "varpush", lineno);
                    (void) hm_get(vars2index, $1, &value);
                    initDSE(&dse, dINTEGER, 0);
                    dse.value.int_v = (long long)value;
                    code(FALSE, STOP, &dse, "variable", lineno);
                    code(TRUE, eval, NULL, "eval", lineno);
                    free($1);
                    $$ = spc;
                  }
                | FIELD {
                    char variable[1024], field[1024], *st;
                    int ndx;
                    void *value;
                    InstructionEntry *spc;
                    field_split($1, variable, field);
                    if (! hm_get(vars2strs, variable, (void **)&st)) {
                      comperror(variable, ": unknown event variable");
                      YYABORT;
                    }
                    if ((ndx = top_index(st, field)) == -1) {
                      comperror(field, ": illegal field name");
                      YYABORT;
                    }
                    spc = code(TRUE, varpush, NULL, "varpush", lineno);
                    (void) hm_get(vars2index, variable, &value);
                    initDSE(&dse, dINTEGER, 0);
                    dse.value.int_v = (long long)value;
                    code(FALSE, STOP, &dse, "event variable", lineno);
                    code(TRUE, constpush, NULL, "constpush", lineno);
                    initDSE(&dse, dINTEGER, 0);
                    dse.value.int_v = ndx;
                    code(FALSE, STOP, &dse, "index", lineno);
                    code(TRUE, extract, NULL, "extract", lineno);
                    free($1);
                    $$ = spc;
                  }
                | assignment
                | pluseq
                | minuseq
                | MAP '(' maptype ')' {
                    code(TRUE, newmap, NULL, "Map", lineno);
                    initDSE(&dse, dINTEGER, 0);
                    dse.value.int_v = $3;
                    code(FALSE, STOP, &dse, "map type", lineno);
                  }
                | WINDOW '(' windowtype ',' winconstr ',' INTEGER ')' {
                    code(TRUE, newwindow, NULL, "Window", lineno);
                    initDSE(&dse, dINTEGER, 0);
                    dse.value.int_v = $3;
                    code(FALSE, STOP, &dse, "window type", lineno);
                    initDSE(&dse, dINTEGER, 0);
                    dse.value.int_v = $5;
                    code(FALSE, STOP, &dse, "constraint type", lineno);
                    initDSE(&dse, dINTEGER, 0);
                    dse.value.int_v = $7;
                    code(FALSE, STOP, &dse, "constraint size", lineno);
                  }
                | FUNCTION '(' argumentlist ')' {
                    if (iflog)
                      fprintf(stderr, "%s called, #args = %lld\n", $1, $3);
                    code(TRUE, function, NULL, "function", lineno);
                    initDSE(&dse, dSTRING, 0);
                    dse.value.str_v = $1;
                    code(FALSE, STOP, &dse, "procname", lineno);
                    initDSE(&dse, dINTEGER, 0);
                    dse.value.int_v = $3;
                    code(FALSE, STOP, &dse, "nargs", lineno);
                  }
                | '(' expr ')' {
                    $$ = $2;
                  }
                | expr '+' expr {
                    $$ = code(TRUE, add, NULL, "add", lineno);
                  }
                | expr '-' expr {
                    $$ = code(TRUE, subtract, NULL, "subtract", lineno);
                  }
                | expr '*' expr {
                    $$ = code(TRUE, multiply, NULL, "multiply", lineno);
                  }
                | expr '/' expr {
                    $$ = code(TRUE, divide, NULL, "divide", lineno);
                  }
                | expr '%' expr {
                    $$ = code(TRUE, modulo, NULL, "modulo", lineno);
                  }
                | expr '|' expr {
                    $$ = code(TRUE, bitOr, NULL, "bitOr", lineno);
                  }
                | expr '&' expr {
                    $$ = code(TRUE, bitAnd, NULL, "bitAnd", lineno);
                  }
                | '-' expr %prec UNARYMINUS {
                    $$ = code(TRUE, negate, NULL, "negate", lineno);
                  }
                | expr GT expr { $$ = code(TRUE, gt, NULL, "gt", lineno); }
                | expr GE expr { $$ = code(TRUE, ge, NULL, "ge", lineno); }
                | expr LT expr { $$ = code(TRUE, lt, NULL, "lt", lineno); }
                | expr LE expr { $$ = code(TRUE, le, NULL, "le", lineno); }
                | expr EQ expr { $$ = code(TRUE, eq, NULL, "eq", lineno); }
                | expr NE expr { $$ = code(TRUE, ne, NULL, "ne", lineno); }
                | expr AND expr { $$ = code(TRUE, and, NULL, "and", lineno); }
                | expr OR expr { $$ = code(TRUE, or, NULL, "or", lineno); }
                | NOT expr { $$ = code(TRUE, not, NULL, "not", lineno); }
                ;
%%

struct fpstruct {
    char *name;
    unsigned int min, max, index;
};

static struct fpstruct functions[] = {
    {"float", 1, 1, 0},		    /* real float(int) */
    {"Identifier", 1, MAX_ARGS, 1},  /* identifier Identifier(arg[, ...]) */
    {"lookup", 2, 2, 2},	    /* map.type lookup(map, identifier) */
    {"average", 1, 1, 3},	    /* real average(window) */
    {"stdDev", 1, 1, 4},	    /* real stdDev(window) */
    {"currentTopic", 0, 0, 5},	    /* string currentTopic() */
    {"Iterator", 1, 1, 6},  	    /* iterator Iterator(map|win|seq) */
    {"next", 1, 1, 7},    	    /* identifier|data next(iterator) */
    {"tstampNow", 0, 0, 8},	    /* tstamp tstampNow() */
    {"tstampDelta", 3, 3, 9},	    /* tstamp tstampDelta(tstamp, int, bool) */
    {"tstampDiff", 2, 2, 10},	    /* int tstampDiff(tstamp, tstamp) */
    {"Timestamp", 1, 1, 11}, 	    /* tstamp Timestamp(string) */
    {"dayInWeek", 1, 1, 12},	    /* int dayInWeek(tstamp) [Sun/0,Sat/6] */
    {"hourInDay", 1, 1, 13},	    /* int hourInDay(tstamp) [0..23] */
    {"dayInMonth", 1, 1, 14},	    /* int dayInMonth(tstamp) [1..31] */
    {"Sequence", 0, MAX_ARGS, 15},   /* sequence Sequence([arg[, ...]]) */
    {"hasEntry", 2, 2, 16},          /* bool hasEntry(map, identifier) */
    {"hasNext", 1, 1, 17},           /* bool hasNext(iterator) */
    {"String", 1, MAX_ARGS, 18},     /* string String(arg[, ...]) */
    {"seqElement", 2, 2, 19},        /* basictype seqElement(seq, int) */
    {"seqSize", 1, 1, 20},           /* int seqSize(seq) */
    {"IP4Addr", 1, 1, 21},           /* int IP4Addr(string) */
    {"IP4Mask", 1, 1, 22},           /* int IP4Mask(int) */
    {"matchNetwork", 3, 3, 23},      /* bool matchNetwork(string, int, int) */
    {"secondInMinute", 1, 1, 24},    /* int secondInMinute(tstamp) [0..60] */
    {"minuteInHour", 1, 1, 25},      /* int minuteInHour(tstamp) [0..59] */
    {"monthInYear", 1, 1, 26},       /* int monthInYear(tstamp) [1..12] */
    {"yearIn", 1, 1, 27},            /* int yearIn(tstamp) [1900 .. ] */
    {"power", 1, 2, 28},             /* real power(real, real) */
    {"winSize", 1, 1, 29},           /* int winSize(win) */
    {"lsqrf", 1, 1, 30},             /* sequence lsqrf(win) */
    {"winMax", 1, 1, 31},            /* real winMax(win) */
    {"floor", 1, 1, 32},             /* int floor(real) */
    {"mapSize", 1, 1, 33},           /* int mapSize(map) */
    {"winElement", 2, 2, 34},        /* win.type winElement(win, int) */
    {"mapMax", 2, 2, 35},            /* ident mapMax(map) */
    {"currentEvent", 0, 0, 36},      /* event currentEvent() */
    {"get", 1, 1, 37},               /* int get(dev_reg) */
    {"set", 2, 2, 38}                /* int set(dev_reg, int) */
};
#define NFUNCTIONS (sizeof(functions)/sizeof(struct fpstruct))

static struct fpstruct procedures[] = {
    {"topOfHeap", 0, 0, 0},     /* void topOfHeap() */
    {"insert", 3, 3, 1},	/* void insert(map, ident, map.type) */
    {"remove", 2, 2, 2},	/* void remove(map, ident) */
    {"send", 1, MAX_ARGS, 3},   /* void send(arg, ...) */
    {"append", 2, MAX_ARGS, 4}, /* void append(window, window.dtype[, tstamp]) */
    /* if wtype == SECS, must provide tstamp */
    /* void append(sequence, basictype[, ...]) */
    {"publish", 2, MAX_ARGS, 5},/* void publish(topic, arg, ...) */
    {"frequent", 3, 3, 6}       /* void frequent(map, ident, int) */
};
#define NPROCEDURES (sizeof(procedures)/sizeof(struct fpstruct))

struct keyval {
    char *key;
    int value;
};

static struct keyval constants[] = {
    {"DEV_reg0", 0x00},
    {"DEV_reg1", 0x01}
};
#define NCONSTANTS (sizeof(constants)/sizeof(struct keyval))

static struct keyval keywords[] = {
    {"subscribe", SUBSCRIBE},
    {"to", TO},
    {"associate", ASSOCIATE},
    {"with", WITH},
    {"bool", BOOLDCL},
    {"int", INTDCL},
    {"real", REALDCL},
    {"string", STRINGDCL},
    {"tstamp", TSTAMPDCL},
    {"sequence", SEQDCL},
    {"iterator", ITERDCL},
    {"map", MAPDCL},
    {"window", WINDOWDCL},
    {"identifier", IDENTDCL},
    {"event", EVENTDCL},
    {"NULL", NULLVAL},
    {"if", IF},
    {"else", ELSE},
    {"while", WHILE},
    {"initialization", INITIALIZATION},
    {"behavior", BEHAVIOR},
    {"Map", MAP},
    {"Window", WINDOW},
    {"destroy", DESTROY},
    {"ROWS", ROWS},
    {"SECS", SECS}
};
#define NKEYWORDS sizeof(keywords)/sizeof(struct keyval)

int get_ch(char **ptr) {
    char *p = *ptr;
    int c = *p;
    if (c)
        p++;
    else
        c = EOF;
    *ptr = p;
    return c;
}

void unget_ch(int c, char **ptr) {
    char *p = *ptr;
    *(--p) = c;
    *ptr = p;
}

int a_lex() {
    int c;
top:
    while ((c = get_ch(&ap)) == ' ' || c == '\t' || c == '\n')
        if (c == '\n')
            lineno++;
    if (c == '#') { 		/* comment to end of line */
        while ((c = get_ch(&ap)) != '\n' && c != EOF)
            ;			/* consume rest of line */
        if (c == '\n') {
            lineno++;
            goto top;
        }
    }
    if (c == EOF)
        return 0;
    if (c == '.' || isdigit(c)) {	/* a number */
        char buf[128], *p;
        double d;
        long l;
        int isfloat = 0;
        int retval;
        p = buf;
        do {
            if (c == '.')
                isfloat++;
            *p++ = c;
            c = get_ch(&ap);
        } while (isdigit(c) || c == '.');
        unget_ch(c, &ap);
        *p = '\0';
        if (isfloat) {
            sscanf(buf, "%lf", &d);
            a_lval.dblv = d;
            retval = DOUBLE;
        } else {
            sscanf(buf, "%ld", &l);
            a_lval.intv = l;
            retval = INTEGER;
        }
        return retval;
    }
    if (c == '@') {	/* timestamp literal */
        char buf[20], *p;
        int n = 16;
        p = buf;
        *p++ = c;
        while (n > 0) {
            c = get_ch(&ap);
            if (! isxdigit(c))
                comperror("syntactically incorrect timestamp", NULL);
            *p++ = c;
            n--;
        }
        c = get_ch(&ap);
        if(c != '@')
            comperror("syntactically incorrect timestamp", NULL);
        *p++ = c;
        *p = '\0';
        a_lval.tstampv = string_to_timestamp(buf);
        return TSTAMP;
    }
    if (isalpha(c)) {
        char sbuf[100], *p = sbuf;
        unsigned int i;
        int isfield = 0;

        do {
            if (p >= sbuf + sizeof(sbuf) - 1) {
                *p = '\0';
                comperror("name too long", sbuf);
            }
            *p++ = c;
            if (c == '.')
                isfield++;
        } while ((c = get_ch(&ap)) != EOF && (isalnum(c) || c == '.' || c == '_'));
        unget_ch(c, &ap);
        *p = '\0';
        for (i = 0; i < NFUNCTIONS; i++) {
            if (strcmp(sbuf, functions[i].name) == 0) {
                a_lval.strv = strdup(sbuf);
                return (FUNCTION);
            }
        }
        for (i = 0; i < NPROCEDURES; i++) {
            if (strcmp(sbuf, procedures[i].name) == 0) {
                a_lval.strv = strdup(sbuf);
                return (PROCEDURE);
            }
        }
        for (i = 0; i < NCONSTANTS; i++) {
            if (strcmp(sbuf, constants[i].key) == 0) {
                a_lval.intv = constants[i].value;
                return (INTEGER);
            }
        }
        if (strcmp(sbuf, "true") == 0) {
            a_lval.intv = 1;
            return (BOOLEAN);
        } else if (strcmp(sbuf, "false") == 0) {
            a_lval.intv = 0;
            return (BOOLEAN);
        }
        for (i = 0; i < NKEYWORDS; i++) {
            if (strcmp(sbuf, keywords[i].key) == 0) {
                return (keywords[i].value);
            }
        }
        if (strcmp(sbuf, "print") == 0)
            return PRINT;
        a_lval.strv = strdup(sbuf);
        if (isfield)
            return FIELD;
        else
            return VAR;
    }
    if (c == '\'') {		/* quoted string */
        char sbuf[100], *p;
        for (p = sbuf; (c = get_ch(&ap)) != '\''; p++) {
            if (c == '\n' || c == EOF)
                comperror("missing quote", NULL);
            if (p >= sbuf + sizeof(sbuf) - 1) {
                *p = '\0';
                comperror("string too long", sbuf);
            }
            *p = backslash(c);
        }
        *p = '\0';
        a_lval.strv = strdup(sbuf);
        return STRING;
    }
    if (c == '<') {	/* parameterized type */
        int d = get_ch(&ap);
        if (isalpha(d)) {	/* if paramtype, next should be alpha */
            unget_ch(d, &ap);
            char sbuf[100], *p;
            for (p = sbuf; (c = get_ch(&ap)) != '>'; p++) {
                if (c == '\n' || c == EOF)
                    comperror("missing >", NULL);
                if (p >= sbuf + sizeof(sbuf) - 1) {
                    *p = '\0';
                    comperror("param too long or space after < needed", sbuf);
                }
                *p = backslash(c);
            }
            *p = '\0';
            a_lval.strv = strdup(sbuf);
            return PARAMTYPE;
        } else {  /* otherwise, it's an expression */
            unget_ch(d, &ap);
        }
    }
    c = backslash(c);
    switch (c) {
    case '>':
        return follow('=', GE, GT);
    case '<':
        return follow('=', LE, LT);
    case '=':
        return follow('=', EQ, '=');
    case '!':
        return follow('=', NE, NOT);
    case '|':
        return follow('|', OR, '|');
    case '&':
        return follow('&', AND, '&');
    case '+':
        return follow('=', PLUSEQ, '+');
    case '-':
        return follow('=', MINUSEQ, '-');
    case '\n':
        lineno++;
        goto top;
    default:
        return c;
    }
}

int backslash(int c) {		/* get next character with \'s interpreted */
    static char transtab[] = "b\bf\fn\nr\rt\t";
    if (c != '\\')
        return c;
    c = get_ch(&ap);
    if (islower(c) && strchr(transtab, c))
        return strchr(transtab, c)[1];
    return c;
}

void field_split(char *token, char *variable, char *field) {
    char *p, *q;
    p = token;
    for (q = variable; *p != '\0'; ) {
        if (*p == '.')
            break;
        *q++ = *p++;
    }
    *q = '\0';
    strcpy(field, (*p == '.') ? ++p : p);
}

int follow(int expect, int ifyes, int ifno) {	/* look ahead for >=, ... */
    int c = get_ch(&ap);
    if (c == expect)
        return ifyes;
    unget_ch(c, &ap);
    return ifno;
}

int a_error(char *s) {		/* report compile-time error */
    comperror(s, (char *)0);
    return 1;
}

void a_init(void) {
    unsigned int i;
    lineno = 1;
    topics = hm_create(25L, 5.0);
    if (vars2strs != NULL)
        hm_destroy(vars2strs, free);
    vars2strs = hm_create(25L, 5.0);
    if (vars2index != NULL)
        hm_destroy(vars2index, NULL);
    vars2index = hm_create(25L, 5.0);
    variables = al_create(25L);
    index2vars = al_create(25L);
    if (builtins == NULL) {
        builtins = hm_create(25L, 5.0);
        for (i = 0; i < NFUNCTIONS; i++) {
            struct fpargs *d = (struct fpargs *)malloc(sizeof(struct fpargs));
            void *dummy;
            if (d != NULL) {
                d->min = functions[i].min;
                d->max = functions[i].max;
                d->index = functions[i].index;
                (void)hm_put(builtins, functions[i].name, d, &dummy);
            }
        }
        for (i = 0; i < NPROCEDURES; i++) {
            struct fpargs *d = (struct fpargs *)malloc(sizeof(struct fpargs));
            void *dummy;
            if (d != NULL) {
                d->min = procedures[i].min;
                d->max = procedures[i].max;
                d->index = procedures[i].index;
                (void)hm_put(builtins, procedures[i].name, d, &dummy);
            }
        }
    }
    if (devconstants == NULL) {
        devconstants = hm_create(25L, 5.0);
        for (i = 0; i < NCONSTANTS; i++) {
            void *dummy;
            (void)hm_put(devconstants, constants[i].key, (void *)constants[i].value, &dummy);
        }
    }
}

void ap_init(char *prog) {
    strcpy(autoprog, prog);
    ap = autoprog;
}

void execerror(int lineno, char *s, char *t) { /* recover from run-time error */
    char buf[1024];
    jmp_buf *begin = (jmp_buf *)pthread_getspecific(jmpbuf_key);
    warning(buf, lineno, s, t);
    (void) pthread_setspecific(execerr_key, (void *)strdup(buf));
    longjmp(*begin, -2);
}

void comperror(char *s, char *t) {	/* recover from compile-time error */
    char buf[1024];
    jmp_buf *begin = (jmp_buf *)pthread_getspecific(jmpbuf_key);
    warning(buf, lineno, s, t);
    (void) pthread_setspecific(execerr_key, (void *)strdup(buf));
    longjmp(*begin, -2);
}

void fpecatch(int __attribute__ ((unused)) signum) { /* catch FP exceptions */
    execerror(0, "floating point exception", (char *)0);
}

static int pack(char *file, char *program) {
    FILE *fp;

    if ((fp = fopen(file, "r")) != NULL) {
        int c;
        char *p = program;
        while ((c = fgetc(fp)) != EOF) {
            *p++ = c;
        }
        *p = '\0';
        fclose(fp);
        return 1;
    }
    return 0;
}

int moreinput(void) {
    if (gargc-- <= 0)
        return 0;
    infile= *gargv++;
    lineno = 1;
    if (! pack(infile, autoprog)) {
        fprintf(stderr, "%s: can't open %s\n", progname, infile);
        return moreinput();
    }
    ap = autoprog;
    return 1;
}


void warning(char *b, int lno, char *s, char *t) { /* print warning message */
    char *p;
    p = b;
    p += sprintf(p, "%s: %s", progname, s);
    if (t)
        p += sprintf(p, " %s", t);
    if (infile)
        p += sprintf(p, " in %s", infile);
    p += sprintf(p, " near line %d\n", lno);
    fputs(b, stderr);
}
