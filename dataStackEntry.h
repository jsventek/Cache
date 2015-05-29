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

#ifndef _DATASTACKENTRY_H_
#define _DATASTACKENTRY_H_

#include "hashmap.h"
#include "linkedlist.h"

/*
 * the data stack for the event processor is an array of the following structs
 */

#define dNULL      0
#define dBOOLEAN   1
#define dINTEGER   2
#define dDOUBLE    3
#define dTSTAMP    4
#define dSTRING    5
#define dEVENT     6
#define dMAP       7
#define dIDENT     8
#define dTIDENT    9
#define dWINDOW   10
#define dITERATOR 11
#define dSEQUENCE 12
#define dPTABLE   13

#define DUPLICATE 1 /* bit indicating that it must be duplicated */
#define MUST_FREE 2 /* bit indicating that must free when done with it */
#define NOTASSIGN 4 /* bit indicating that it cannot be assigned*/

#define dROWS 21
#define dSECS 22

#define initDSE(d,t,f) {(d)->type=(t); (d)->flags=(f); }

typedef struct gaplmap {
    int type;
    HashMap *hm;
} GAPLMap;

typedef struct gaplwindow {
    int dtype;
    int wtype;
    unsigned int rows_secs;
    LinkedList *ll;
} GAPLWindow;

typedef struct gapliterator {
    int type;
    int dtype;      /* needed for window iterator */
    int next;
    int size;
    union {
        char **m_idents;
        Iterator *w_it;
    } u;
} GAPLIterator;

typedef struct gaplsequence GAPLSequence;

typedef struct dataStackEntry {
    unsigned short type, flags;
    union {
        int bool_v;
        signed long long int_v;
        double dbl_v;
        unsigned long long tstamp_v;
        char *str_v;
        void *ev_v;
        GAPLMap *map_v;
        GAPLWindow *win_v;
        GAPLIterator *iter_v;
        GAPLSequence *seq_v;
        struct dataStackEntry *next;    /* used to link onto free list */
    } value;
} DataStackEntry;

typedef struct gaplwindowEntry {
    unsigned long long tstamp;
    DataStackEntry dse;
} GAPLWindowEntry;

struct gaplsequence {
    int used;
    int size;
    DataStackEntry *entries;
};

#endif /* _DATASTACKENTRY_H_ */
