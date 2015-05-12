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
 * implementation for generic linked list
 */

#include "linkedlist.h"
#include <stdlib.h>

#define SENTINEL(p) (&(p)->sentinel)
#define FL_INCREMENT 128	/* number of entries to add to free list */

typedef struct llnode {
    struct llnode *next;
    struct llnode *prev;
    void *element;
} LLNode;

struct linkedlist {
    long size;
    LLNode *freel;
    LLNode sentinel;
};

/*
 * local routines for maintaining free list of LLNode's
 */

static void putEntry(LinkedList *ll, LLNode *p) {
    p->element = NULL;
    p->next = ll->freel;
    ll->freel = p;
}

static LLNode *getEntry(LinkedList *ll) {
    LLNode *p;

    if ((p = ll->freel) == NULL) {
        long i;
        for (i = 0; i < FL_INCREMENT; i++) {
            p = (LLNode *)malloc(sizeof(LLNode));
            if (p == NULL)
                break;
            putEntry(ll, p);
        }
        p = ll->freel;
    }
    if (p != NULL)
        ll->freel = p->next;
    return p;
}

LinkedList *ll_create(void) {
    LinkedList *ll;

    ll = (LinkedList *)malloc(sizeof(LinkedList));
    if (ll != NULL) {
        ll->size = 0l;
        ll->freel = NULL;
        ll->sentinel.next = SENTINEL(ll);
        ll->sentinel.prev = SENTINEL(ll);
    }
    return ll;
}

/*
 * traverses linked list, calling userFunction on each element and freeing
 * node associated with element
 */
static void purge(LinkedList *ll, void (*userFunction)(void *element)) {
    LLNode *cur = ll->sentinel.next;

    while (cur != SENTINEL(ll)) {
        LLNode *next;
        if (userFunction != NULL)
            (*userFunction)(cur->element);
        next = cur->next;
        putEntry(ll, cur);
        cur = next;
    }
}

void ll_destroy(LinkedList *ll, void (*userFunction)(void *element)) {
    LLNode *p;
    purge(ll, userFunction);
    p = ll->freel;
    while (p != NULL) {		/* return nodes on free list */
        LLNode *q;
        q = p->next;
        free(p);
        p = q;
    }
    free(ll);
}

/*
 * link `p' between `before' and `after'
 * must work correctly if `before' and `after' are the same node
 * (i.e. the sentinel)
 */
static void link(LLNode *before, LLNode *p, LLNode *after) {
    p->next = after;
    p->prev = before;
    after->prev = p;
    before->next = p;
}

int ll_add(LinkedList *ll, void *element) {
    return ll_addLast(ll, element);
}

int ll_insert(LinkedList *ll, long index, void *element) {
    int status = 0;
    LLNode *p;

    if (index <= ll->size && (p = getEntry(ll)) != NULL) {
        long n;
        LLNode *b;

        p->element = element;
        status = 1;
        for (n = 0, b = SENTINEL(ll); n < index; n++, b = b->next)
            ;
        link(b, p, b->next);
        ll->size++;
    }
    return status;
}

int ll_addFirst(LinkedList *ll, void *element) {
    int status = 0;
    LLNode *p = getEntry(ll);

    if (p != NULL) {
        p->element = element;
        status = 1;
        link(SENTINEL(ll), p, SENTINEL(ll)->next);
        ll->size++;
    }
    return status;
}

int ll_addLast(LinkedList *ll, void *element) {
    int status = 0;
    LLNode *p = getEntry(ll);

    if (p != NULL) {
        p->element = element;
        status = 1;
        link(SENTINEL(ll)->prev, p, SENTINEL(ll));
        ll->size++;
    }
    return status;
}

void ll_clear(LinkedList *ll, void (*userFunction)(void *element)) {
    purge(ll, userFunction);
    ll->size = 0L;
    ll->sentinel.next = SENTINEL(ll);
    ll->sentinel.prev = SENTINEL(ll);
}

int ll_get(LinkedList *ll, long index, void **element) {
    int status = 0;

    if (index < ll->size) {
        long n;
        LLNode *p;

        status = 1;
        for (n = 0, p = SENTINEL(ll)->next; n < index; n++, p = p->next)
            ;
        *element = p->element;
    }
    return status;
}

int ll_getFirst(LinkedList *ll, void **element) {
    int status = 0;
    LLNode *p = SENTINEL(ll)->next;

    if (p != SENTINEL(ll)) {
        status = 1;
        *element = p->element;
    }
    return status;
}

int ll_getLast(LinkedList *ll, void **element) {
    int status = 0;
    LLNode *p = SENTINEL(ll)->prev;

    if (p != SENTINEL(ll)) {
        status = 1;
        *element = p->element;
    }
    return status;
}

/*
 * unlinks the LLNode from the doubly-linked list
 */
static void unlink(LLNode *p) {
    p->prev->next = p->next;
    p->next->prev = p->prev;
}

int ll_remove(LinkedList *ll, long index, void **element) {
    int status = 0;

    if (index < ll->size) {
        long n;
        LLNode *p;

        status = 1;
        for (n = 0, p = SENTINEL(ll)->next; n < index; n++, p = p->next)
            ;
        *element = p->element;
        unlink(p);
        putEntry(ll, p);
        ll->size--;
    }
    return status;
}

int ll_removeFirst(LinkedList *ll, void **element) {
    int status = 0;
    LLNode *p = SENTINEL(ll)->next;

    if (p != SENTINEL(ll)) {
        status = 1;
        *element = p->element;
        unlink(p);
        putEntry(ll, p);
        ll->size--;
    }
    return status;
}

int ll_removeLast(LinkedList *ll, void **element) {
    int status = 0;
    LLNode *p = SENTINEL(ll)->prev;

    if (p != SENTINEL(ll)) {
        status = 1;
        *element = p->element;
        unlink(p);
        putEntry(ll, p);
        ll->size--;
    }
    return status;
}

int ll_set(LinkedList *ll, long index, void *element, void **previous) {
    int status = 0;

    if (index < ll->size) {
        long n;
        LLNode *p;

        status = 1;
        for (n = 0, p = SENTINEL(ll)->next; n < index; n++, p = p->next)
            ;
        *previous = p->element;
        p->element = element;
    }
    return status;
}

long ll_size(LinkedList *ll) {
    return ll->size;
}

int ll_isEmpty(LinkedList *ll) {
    return (ll->size == 0L);
}

/*
 * local function to generate array of element values on the heap
 *
 * returns pointer to array or NULL if malloc failure
 */
static void **genArray(LinkedList *ll) {
    void **tmp = NULL;
    if (ll->size > 0L) {
        size_t nbytes = ll->size * sizeof(void *);
        tmp = (void **)malloc(nbytes);
        if (tmp != NULL) {
            long i;
            LLNode *p;
            for (i = 0, p = SENTINEL(ll)->next; i < ll->size; i++, p = p->next)
                tmp[i] = p->element;
        }
    }
    return tmp;
}

void **ll_toArray(LinkedList *ll, long *len) {
    void **tmp = genArray(ll);

    if (tmp != NULL)
        *len = ll->size;
    return tmp;
}

Iterator *ll_it_create(LinkedList *ll) {
    Iterator *it = NULL;
    void **tmp = genArray(ll);

    if (tmp != NULL) {
        it = it_create(ll->size, tmp);
        if (it == NULL)
            free(tmp);
    }
    return it;
}
