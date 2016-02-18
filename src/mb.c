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

/*
 * mb.c - source file for circular buffer that underlies the Homework DB
 *
 * the buffer is allocated statically with size MB_SIZE
 *
 * tuples are allocated starting at the beginning of the buffer
 *
 * nodes are allocated from the end of the buffer
 *
 * after the buffer is exhausted, treat tuple space as a circular buffer
 * and the system is constrained to the number of nodes that exhausted the
 * buffer
 */

#ifndef ALIGNMENT	/* override if you know better! */
#define ALIGNMENT (sizeof(void *))
#endif /* ALIGNMENT */

/*
 * max value for MB_SIZE_IN_ALIGHMENT_UNITS is 240000000
 */
#ifndef MB_SIZE_IN_ALIGNMENT_UNITS
#define MB_SIZE_IN_ALIGNMENT_UNITS 24000000
#endif /* MB_SIZE_IN_ALIGNMENT_UNITS */

/* default is 1,600,000,000 bytes for 32-bit, 3,200,000,000 for 64-bit */
#define MB_SIZE (MB_SIZE_IN_ALIGNMENT_UNITS * ALIGNMENT)

/*
 * we stop allocating Node structures when this much space is all that is left
 * above the last allocated tuple on the first pass through the buffer
 */
#define BUFFER_ZONE 512 * ALIGNMENT

/*
 * compute aligned size of Node
 */
#define ALIGNED_NODE_SIZE (((sizeof(Node) - 1) / ALIGNMENT + 1) * ALIGNMENT)

/*
 * minimum number of Nodes to allocate at a time
 */
#define MINIMUM_NODES 25

/*
 * macro for appending a structure to a singly linked list
 * elem - pointer to the structure to append
 * head - pointer to the head of the list
 * tail - pointer to the tail of the list
 * link - expression for the link to the next element in tail
 * count - a variable that maintains the number of elements in the list
 *
 * assumes elem->next has already been set to NULL
 */

#define append2LL(elem, head, tail, link, count) {\
	if ((count)++) (link) = (elem); else (head) = (elem); \
	(tail) = (elem); }

#include "mb.h"
#include "node.h"
#include "table.h"
#include "tuple.h"
#include "timestamp.h"
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/time.h>
#include <stdlib.h>

static unsigned char mb[MB_SIZE];	/* the memory buffer */
static unsigned char *oldestT = mb;	/* address of oldest tuple */
static unsigned char *nextT = mb + ALIGNMENT;	/* byte for next tuple */
static long nbytes = ALIGNMENT;		/* number of bytes used */
static long lastIndex = MB_SIZE - ALIGNED_NODE_SIZE;	/* last index used for node alloc */
static unsigned char *lastPtr = mb + MB_SIZE - ALIGNED_NODE_SIZE;	/* address of mb[lastIndex] */
static int partitionFixed = 0;		/* set to 1 when buffer exhausted */
static Node *freeN = NULL;		/* free list of nodes */
static Node *firstN;			/* least recently allocated node */
static Node *lastN;			/* most recently allocated node */
static long nnodes = 1L;		/* number of nodes in use */
static Table dTbl;			/* dummy table to hold dummy tuple */
static long passes = 0L;		/* counter of passes through buffer */
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * allocate another block of Nodes, working down from high memory
 *
 * computes the number of (Tuple+Node)'s that would fit in 25% of the space
 * remaining; when that number drops to zero, makes partitionFixed TRUE
 *
 * if ifFirst is true, assumes average tuple size is 24 bytes
 * if false, computes the average tuple size of stored tuples
 *
 * Still have a check to see if the allocation makes its way into the buffer
 * zone, and if so, sets partitionFixed and stops
 *
 * this should not happen
 */
static void alloc_block(int ifFirst) {
    long nNodes;
    long i, ind;
    long zoneIndex;
    Node *t;
    long tupleAverage;
    if (partitionFixed)			/* no more nodes can be alloced */
        return;
    /* have to account for tuple space + Node space */
    if (ifFirst)	/* assume tuple average is 24 + ALIGNED_NODE_SIZE) */
        tupleAverage = 24 + ALIGNED_NODE_SIZE;
    else		/* compute tuple average from nbytes and nnodes */
        tupleAverage = nbytes / nnodes + ALIGNED_NODE_SIZE;
    nNodes = (long)(lastPtr - nextT) / 4 / tupleAverage;
    if (nNodes <= 0) {
        partitionFixed++;
        return;
    } else if (nNodes < MINIMUM_NODES)
        nNodes = MINIMUM_NODES;
    zoneIndex = (long)(nextT - mb) + BUFFER_ZONE;
    for (i = 0L; i < nNodes; i++) {
        ind = lastIndex - ALIGNED_NODE_SIZE;
        if (ind < zoneIndex) {		/* node in buffer zone, stop */
            partitionFixed++;
            break;
        }
        lastIndex = ind;		/* add node to free list */
        lastPtr = &(mb[lastIndex]);
        t = (Node *)lastPtr;
        t->next = freeN;
        freeN = t;
    }
}

/*
 * allocate a node from the free list
 *
 * if free list is empty, attempt to allocate another block of Nodes
 * and try again
 *
 * return NULL if no more free nodes
 */
static Node *alloc_node() {
    Node *p;
    if (!freeN && !partitionFixed)
        alloc_block(0);
    if ((p = freeN))
        freeN = p->next;
    return p;
}

/*
 * free oldest node, cleaning up the data structures
 */
static void free_node() {
    Node *t = firstN;		/* least-recently allocated tuple */
    firstN = t->younger;	/* unlink it from active list */
    oldestT = firstN->tuple;	/* oldestT now points to new oldest tuple */
    nbytes -= t->alloc_len;	/* update bytes allocated */
    Table *tb = t->parent;	/* locate the table holding tuple */
    Node *u = t->next;
    tb->oldest = u;		/* remove from table */
    if (!(--(tb->count)))	/* list now empty */
        tb->newest = NULL;
    else
        u->prev = NULL;
    t->next = freeN;		/* return Node to free list */
    freeN = t;
    nnodes--;			/* update nodes in use */

}

/*
 * mb_init() - initialize the circular buffer and node free pool
 *
 * initializes the dummy tuple, then generates the first block of Nodes
 * in the free pool
 */
void mb_init() {
    (void) pthread_mutex_lock(&mutex);
    firstN = (Node *)lastPtr;	/* least recently allocated node */
    lastN = (Node *)lastPtr;	/* most recently allocated node */
    firstN->parent = &dTbl;	/* fill in dummy tuple and table */
    firstN->next = NULL;
    firstN->younger = NULL;
    firstN->alloc_len = ALIGNMENT;
    firstN->real_len = ALIGNMENT;
    firstN->tuple = oldestT;
    (void) pthread_mutex_init(&(dTbl.tb_mutex), NULL);
    (void) pthread_mutex_lock(&(dTbl.tb_mutex));
    append2LL(firstN, dTbl.oldest, dTbl.newest, (dTbl.newest)->next, dTbl.count);
    (void) pthread_mutex_unlock(&(dTbl.tb_mutex));
    alloc_block(1);	/* allocate initial tranche of Nodes */
    (void) pthread_mutex_unlock(&mutex);
}

/*
 * mb_insert - insert buffer into the circular buffer
 *
 * return 1 if successful, 0 if not
 */
int mb_insert(unsigned char *buf, long len, Table *tb) {
    Node *n;
    unsigned short alloc_len = ((len - 1) / ALIGNMENT + 1) * ALIGNMENT;
    unsigned char *t;
    struct timeval tv;
    (void) pthread_mutex_lock(&mutex);
    while (!(n = alloc_node()))
        free_node();			/* free up oldest node */
    for (;;) {
        if (oldestT < nextT) {		/* oldestT behind nextT */
            if ((nextT + alloc_len) >= lastPtr) {
                free_node();	/* oldest node must be at mb or later */
                nextT = mb;	/* reset pointer */
                passes++;
            } else
                break; /* OK */
        } else {		/* oldestT behind nextT */
            if ((nextT + alloc_len >= oldestT)) {
                free_node();
            } else
                break;	/* OK */
        }
    }
    /*
     * at this point, we have a node (n) and nextT points at location in
     * buffer big enough to hold the tuple
     */
    t = nextT;
    nextT += alloc_len;		/* now point at next free location */
    nbytes += alloc_len;	/* update the bytes in use counter */
    n->parent = tb;		/* fill in node member data */
    n->next = NULL;
    n->prev = NULL;
    n->younger = NULL;
    n->alloc_len = alloc_len;
    n->real_len = (unsigned short)len;
    n->tuple = t;
    (void) gettimeofday(&tv, NULL);		/* timestamp the tuple */
    n->tstamp = timeval_to_timestamp(&tv);
    memcpy(t, buf, len);	/* copy buf to t */
    append2LL(n, firstN, lastN, lastN->younger, nnodes);
    (void) pthread_mutex_lock(&(tb->tb_mutex));
    if ((tb->count)++) {	/* list was not empty */
        tb->newest->next = n;
        n->prev = tb->newest;
        tb->newest = n;
    } else {
        tb->newest = n;
        tb->oldest = n;
    }
    (void) pthread_mutex_unlock(&(tb->tb_mutex));
    (void) pthread_mutex_unlock(&mutex);

    return 1;
}

/*
 * mb_insert_tuple - insert tuple into the circular buffer
 *
 * return timestamp if successful, (tstamp_t)0 if not
 */
tstamp_t mb_insert_tuple(int ncols, char *vals[], Table *tb) {
    Node *n;
    int len = ncols * sizeof(char *);
    int i;
    unsigned short alloc_len;
    unsigned char *t, *s;
    union Tuple *p;
    struct timeval tv;
    tstamp_t ts;

    for (i = 0; i < ncols; i++)
        len += strlen(vals[i]) + 1;
    alloc_len = ((len - 1) / ALIGNMENT + 1) * ALIGNMENT;
    (void) pthread_mutex_lock(&mutex);
    while (!(n = alloc_node()))
        free_node();			/* free up oldest node */
    for (;;) {
        if (oldestT < nextT) {		/* oldestT behind nextT */
            if ((nextT + alloc_len) >= lastPtr) {
                free_node();	/* oldest node must be at mb or later */
                nextT = mb;	/* reset pointer */
                passes++;
            } else
                break; /* OK */
        } else {		/* oldestT behind nextT */
            if ((nextT + alloc_len >= oldestT)) {
                free_node();
            } else
                break;	/* OK */
        }
    }
    /*
     * at this point, we have a node (n) and nextT points at location in
     * buffer big enough to hold the tuple
     */
    t = nextT;
    nextT += alloc_len;		/* now point at next free location */
    nbytes += alloc_len;	/* update the bytes in use counter */
    n->parent = tb;		/* fill in node member data */
    n->next = NULL;
    n->prev = NULL;
    n->younger = NULL;
    n->alloc_len = alloc_len;
    n->real_len = (unsigned short)len;
    n->tuple = t;
    (void) gettimeofday(&tv, NULL);		/* timestamp the tuple */
    ts = timeval_to_timestamp(&tv);
    n->tstamp = ts;
    p = (union Tuple *)t;
    t += ncols * sizeof(char *);
    for (i = 0; i < ncols; i++) {
        p->ptrs[i] = (char *)t;
        s = (unsigned char *)vals[i];
        while ((*t++ = *s++))
            ;
    }
    append2LL(n, firstN, lastN, lastN->younger, nnodes);
    (void) pthread_mutex_lock(&(tb->tb_mutex));
    if ((tb->count)++) {	/* list was not empty */
        tb->newest->next = n;
        n->prev = tb->newest;
        tb->newest = n;
    } else {
        tb->newest = n;
        tb->oldest = n;
    }
    (void) pthread_mutex_unlock(&(tb->tb_mutex));
    (void) pthread_mutex_unlock(&mutex);

    return ts;
}

tstamp_t heap_insert_tuple(int ncols, char *vals[], Table *tb, Node *node) {

    Node *n;
    struct timeval tv;
    tstamp_t ts;

    int i;

    unsigned char *t, *s, *buf;
    union Tuple *p;

    unsigned short alloc_len;
    int len = ncols * sizeof(char *);
    for (i = 0; i < ncols; i++)
        len += strlen(vals[i]) + 1;
    alloc_len = ((len - 1) / ALIGNMENT + 1) * ALIGNMENT;

    buf = malloc(alloc_len);
    if (!buf) {
        printf("Out of memory\n");
        return (tstamp_t)0;
    };
    if (! (n = node)) {
        n = malloc(sizeof(Node));
        if (!n) {
            printf("Out of memory\n");
            free(buf);
            return (tstamp_t)0;
        }
    }

    p = (union Tuple *) buf;
    t = buf + ncols * sizeof(char *);
    for (i = 0; i < ncols; i++) {
        p->ptrs[i] = (char *) t;
        s = (unsigned char *) vals[i];
        while ((*t++ = *s++))
            ;
    }

    (void) pthread_mutex_lock(&mutex);
    (void) pthread_mutex_lock(&(tb->tb_mutex));
    if (node) {	/* must remove node from list & return previous tuple */
        /* remove node from list */
        if (tb->oldest == tb->newest) { /* == node */
            tb->oldest = NULL;
            tb->newest = NULL;
        } else if (tb->oldest == node) {
            tb->oldest = node->next;
            node->next->prev = NULL;
        } else if (tb->newest == node) {
            tb->newest = node->prev;
            node->prev->next = NULL;
        } else {
            node->prev->next = node->next;
            node->next->prev = node->prev;
        }
        --tb->count;

        free(node->tuple);
    }
    /* fill in node member data */
    n->parent = tb;
    n->next = NULL;
    n->prev = NULL;
    n->younger = NULL;
    n->alloc_len = alloc_len;
    n->real_len = (unsigned short) len;
    n->tuple = buf;
    (void) gettimeofday(&tv, NULL); /* timestamp the tuple */
    ts = timeval_to_timestamp(&tv);
    n->tstamp = ts;
    if ((tb->count)++) { /* list was not empty */
        tb->newest->next = n;
        n->prev = tb->newest;
        tb->newest = n;
    } else {
        tb->newest = n;
        tb->oldest = n;
    }
    (void) pthread_mutex_unlock(&(tb->tb_mutex));
    (void) pthread_mutex_unlock(&mutex);
    return ts;
}

Node *heap_alloc_node(int ncols, char *vals[], Table *tb) {

    Node *n;
    struct timeval tv;

    int i;

    unsigned char *t, *s;
    union Tuple *p;

    unsigned short alloc_len;
    int len = ncols * sizeof(char *);
    for (i = 0; i < ncols; i++)
        len += strlen(vals[i]) + 1;
    alloc_len = ((len - 1) / ALIGNMENT + 1) * ALIGNMENT;

    n = malloc(sizeof(Node));
    if (!n) {
        printf("Out of memory\n");
        return NULL;
    }
    t = malloc(alloc_len);
    if (!t) {
        printf("Out of memory\n");
        free(n);
        return NULL;
    };

    /* fill in node member data */
    n->parent = tb;
    n->next = NULL;
    n->prev = NULL;
    n->younger = NULL;
    n->alloc_len = alloc_len;
    n->real_len = (unsigned short) len;
    n->tuple = t;
    (void) gettimeofday(&tv, NULL); /* timestamp the tuple */
    n->tstamp = timeval_to_timestamp(&tv);

    p = (union Tuple *) t;
    t += ncols * sizeof(char *);
    for (i = 0; i < ncols; i++) {
        p->ptrs[i] = (char *) t;
        s = (unsigned char *) vals[i];
        while ((*t++ = *s++))
            ;
    }
    return n;
}

void heap_remove_node(Node *n, Table *tn) {

    union Tuple *p;

    p = (union Tuple *)(n->tuple);
    /* remove n from list */
    if (tn->oldest == tn->newest) { /* == n */
        tn->oldest = NULL;
        tn->newest = NULL;
    } else if (tn->oldest == n) {
        tn->oldest = n->next;
        n->next->prev = NULL;
    } else if (tn->newest == n) {
        tn->newest = n->prev;
        n->prev->next = NULL;
    } else {
        n->prev->next = n->next;
        n->next->prev = n->prev;
    }
    --tn->count;

    free(p);
    free(n);
}

void mb_dump() {
    long bnodes, total, unused;
    (void) pthread_mutex_lock(&mutex);
    bnodes = nnodes * ALIGNED_NODE_SIZE;
    total = nbytes + bnodes;
    unused = MB_SIZE - total;
    printf("bytes used for tuples = %ld\n", nbytes);
    printf("bytes used for %ld nodes = %ld\n", nnodes, bnodes);
    printf("average bytes per tuple = %.2f\n", (double)total / (double)nnodes);
    printf("unused bytes in table %ld\n", unused);
    printf("completed passes through the circular buffer %ld\n", passes);
    (void) pthread_mutex_unlock(&mutex);
}
