%{
#define YY_NO_INPUT
%}

%{
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "gram.h"

extern YYSTYPE yylval;
%}

%option nounput

%%
SELECT			{ return SELECT; }
select			{ return SELECT; }
FROM			{ return FROM; }
from			{ return FROM; }
WHERE			{ return WHERE; }
where			{ return WHERE; }
CREATE			{ return CREATE; }
create			{ return CREATE; }
TABLE			{ return TABLETK; }
table			{ return TABLETK; }
PERSISTENTTABLE	{ return PERSISTENTTABLETK; }
persistenttable	{ return PERSISTENTTABLETK; }
INSERT			{ return INSERT; }
insert			{ return INSERT; }
INTO			{ return INTO;}
into			{ return INTO;}
VALUES			{ return VALUES;}
values			{ return VALUES;}

DELETE                  { return DELETE; }
delete                  { return DELETE; }

update			{ return UPDATE; }
UPDATE			{ return UPDATE; }
set				{ return SET; }
SET				{ return SET; }

SHOW			{ return SHOW;}
show			{ return SHOW;}
TABLES			{ return TABLES;}
tables			{ return TABLES;}

ROWS			{ return ROWS;}
rows			{ return ROWS;}
RANGE			{ return RANGE;}
range			{ return RANGE;}
NOW			{ return NOW;}
now			{ return NOW;}
SINCE			{ return SINCE;}
since			{ return SINCE;}
INTERVAL		{ return INTERVAL;}
interval		{ return INTERVAL;}
SECONDS			{ return SECONDS;}
seconds			{ return SECONDS;}
MINUTES			{ return MINUTES;}
minutes			{ return MINUTES;}
HOURS			{ return HOURS;}
hours			{ return HOURS;}
LAST			{ return LAST;}
last			{ return LAST;}
MILLIS			{ return MILLIS;}
millis			{ return MILLIS;}


boolean			{ return BOOLEAN;}
integer			{ return INTEGER;}
real			{ return REAL;}
character		{ return CHARACTER;}
varchar			{ return VARCHAR;}
blob			{ return BLOB;}
tinyint			{ return TINYINT;}
smallint		{ return SMALLINT;}
tstamp			{ return TSTAMP;}
primary			{ return PRIMARY;}
PRIMARY			{ return PRIMARY;}
key			{ return KEY;}
KEY			{ return KEY;}

true			{ return TRUETK;}
false			{ return FALSETK;}

and			{ return AND; }
AND			{ return AND; }
or			{ return OR; }
OR			{ return OR; }

count			{ return COUNT; }
COUNT			{ return COUNT; }
min			{ return MIN; }
MIN			{ return MIN; }
max			{ return MAX; }
MAX			{ return MAX; }
avg			{ return AVG; }
AVG			{ return AVG; }
sum			{ return SUM; }
SUM			{ return SUM; }

group			{ return GROUP; }
GROUP			{ return GROUP; }
order			{ return ORDER; }
ORDER			{ return ORDER; }
by			{ return BY; }
BY			{ return BY; }

register		{ return REGISTER; }
REGISTER		{ return REGISTER; }
unregister		{ return UNREGISTER; }
UNREGISTER		{ return UNREGISTER; }

CONTAINS		{ return CONTAINS; }
contains		{ return CONTAINS; }
NOTCONTAINS		{ return NOTCONTAINS; }
notcontains		{ return NOTCONTAINS; }

on 				{ return ON; }
ON 				{ return ON; }
duplicate 		{ return DUPLICATETK; }
DUPLICATE 		{ return DUPLICATETK; }


[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+ { yylval.string = strdup(yytext); return IPADDR; }

%[0-9]+\.[0-9]+		{ yylval.numfloat = atof(yytext); return NUMFLOAT; }
%[0-9]+			{ yylval.number = strtoll(yytext, NULL, 10); return NUMBER; }

[0-9]+\.[0-9]+		{ yylval.string = strdup(yytext); return NUMFLOAT; }
\-[0-9]+\.[0-9]+	{ yylval.string = strdup(yytext); return NUMFLOAT; }
[0-9]+			{ yylval.string = strdup(yytext); return NUMBER; }
\-[0-9]+		{ yylval.string = strdup(yytext); return NUMBER; }
\@[0-9a-fA-F]{16}\@	{ yylval.string = strdup(yytext); return TSTAMP; }
[0-9]{4}\/[0-9]{1,2}\/[0-9]{1,2}\:[0-9]{2}\:[0-9]{2}\:[0-9]{2}	{ yylval.string = strdup(yytext); return DATESTRING; }

[a-zA-Z]+[a-zA-Z0-9\.\-]*	{ yylval.string = strdup(yytext); return WORD; }

\"[^"\n]*["\n]          { yylval.string = strdup(yytext); return QUOTEDSTRING; }

\'			{ return SINGLEQUOTE; }
\<=			{ return LESSEQ; }
\>=			{ return GREATEREQ; }
\<			{ return LESS; }
\>			{ return GREATER; }
=			{ return EQUALS; }
\+=			{ return ADD; }
\-=			{ return SUB; }
,			{ return COMMA; }
\*			{ return STAR; }
;			{ return SEMICOLON; }
\(			{ return OPENBRKT; }
\)			{ return CLOSEBRKT;}
\[			{ return OPENSQBRKT;}
\]			{ return CLOSESQBRKT;}
"//".*\n		/* ignore comments */
\n			/* ignore EOL */
[ \t]+			/* ignore whitespace */
%%
