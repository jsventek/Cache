/*
 * The Homework Database
 *
 * SQL Parser
 *
 * Authors:
 *    Oliver Sharma and Joe Sventek
 *     {oliver, joe}@dcs.gla.ac.uk
 *
 * (c) 2009. All rights reserved.
 */
#ifndef HWDB_PARSER_H
#define HWDB_PARSER_H

#include "sqlstmts.h"
#include "gram.h"

/* Places parsed output in externally declared global variable:
 *     sqlstmt stmt
 */
void *sql_parse(char *query);

void reset_statement(void);

void sql_reset_parser(void *bufstate);

void sql_dup_stmt(sqlstmt *dup);

/* Prints externally declared global variable
 *     sqlstmt stmt
 * to standard output
 */
void sql_print();

#endif
