/*
 * node.h - type definition for node in a table
 */
#ifndef _NODE_H_
#define _NODE_H_

#include "table.h"
#include "timestamp.h"

typedef struct node {
    struct node *next;		/* link to next node in the table */
    struct node *prev;		/* link to previous node in the table */
    struct node *younger;	/* link to next younger node */
    unsigned char *tuple;	/* pointer to Tuple in circ buffer */
    unsigned short alloc_len;	/* bytes allocated for tuple in circ buffer */
    unsigned short real_len;	/* actual lengthof the tuple in bytes */
    struct table *parent;	/* table to which node belongs */
    tstamp_t tstamp;		/* timestamp when entered into database
                                   nanoseconds since epoch */
} Node;

#endif /* _NODE_H_ */
