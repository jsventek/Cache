/*
 * mb.h - publicly accessible entry points for the memory buffer
 */
#ifndef _MB_H_
#define _MB_H_

#include "tuple.h"
#include "table.h"

void mb_init();

int mb_insert(unsigned char *buf, long len, Table *table);

int mb_insert_tuple(int ncols, char *vals[], Table *table);

int heap_insert_tuple(int ncols, char *vals[], Table *table, Node *n);
Node *heap_alloc_node(int ncols, char *vals[], Table *table);
void heap_remove_node(Node *n, Table *tn);
void mb_dump();

#endif /* _MB_H_ */
