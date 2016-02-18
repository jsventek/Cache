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

#include "dsemem.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define INCREMENT 4096	/* number added to free list when depleted */

/*
 * private structures used to represent a linked list and list entries
 */

static DataStackEntry *freeList = NULL;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/* assumes that mutex is held */
static void addToFreeList(DataStackEntry *p) {
    p->value.next = freeList;
    freeList = p;
}

void dse_free(DataStackEntry *p) {
    pthread_mutex_lock(&mutex);
    addToFreeList(p);
    pthread_mutex_unlock(&mutex);
}

DataStackEntry *dse_alloc(void) {
    DataStackEntry *p;

    pthread_mutex_lock(&mutex);
    if (!(p = freeList)) {
        p = (DataStackEntry *)malloc(INCREMENT * sizeof(DataStackEntry));
        if (p) {
            int i;

            for (i = 0; i < INCREMENT; i++, p++)
                addToFreeList(p);
            p = freeList;
        }
    }
    if (p)
        freeList = p->value.next;
    pthread_mutex_unlock(&mutex);
    if (p)
        (void)memset(p, 0, sizeof(DataStackEntry));
    return p;
}
