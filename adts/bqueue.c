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
 * implementation for generic bounded FIFO queue
 */

#include "bqueue.h"
#include <stdlib.h>

struct bqueue {
    long count;
    long size;
    int in;
    int out;
    void **buffer;
};

BQueue *bq_create(long capacity) {
    BQueue *bq = (BQueue *)malloc(sizeof(BQueue));

    if (bq != NULL) {
        long cap = capacity;
        void **tmp;

        if (cap <= 0L)
            cap = DEFAULT_CAPACITY;
        else if (cap > MAX_CAPACITY)
            cap = MAX_CAPACITY;
        tmp = (void **)malloc(cap * sizeof(void *));
        if (tmp == NULL) {
            free(bq);
            bq = NULL;
        } else {
            int i;

            bq->count = 0;
            bq->size = cap;
            bq->in = 0;
            bq->out = 0;
            for (i = 0; i < cap; i++)
                tmp[i] = NULL;
            bq->buffer = tmp;
        }
    }
    return bq;
}

static void purge(BQueue *bq, void (*userFunction)(void *element)) {
    if (userFunction != NULL) {
        int i, n;

        for (i = bq->out, n = bq->count; n > 0; i = (i + 1) % bq->size, n--)
            (*userFunction)(bq->buffer[i]);
    }
}

void bq_destroy(BQueue *bq, void (*userFunction)(void *element)) {
    purge(bq, userFunction);
    free(bq->buffer);
    free(bq);
}

void bq_clear(BQueue *bq, void (*userFunction)(void *element)) {
    int i;

    purge(bq, userFunction);
    for (i = 0; i < bq->size; i++)
        bq->buffer[i] = NULL;
    bq->count = 0;
    bq->in = 0;
    bq->out = 0;
}

int bq_add(BQueue *bq, void *element) {
    int i;
    
    if (bq->count == bq->size)
        return 0;
    i = bq->in;
    bq->buffer[i] = element;
    bq->in = (i + 1) % bq->size;
    bq->count++;
    return 1;
}

static int retrieve(BQueue *bq, void **element, int ifRemove) {
    int i;

    if (bq->count <= 0)
        return 0;
    i = bq->out;
    *element = bq->buffer[i];
    if (ifRemove) {
        bq->out = (i + 1) % bq->size;
        bq->count--;
    }
    return 1;
}

int bq_peek(BQueue *bq, void **element) {
    return retrieve(bq, element, 0);
}

int bq_remove(BQueue *bq, void **element) {
    return retrieve(bq, element, 1);
}

long bq_size(BQueue *bq) {
    return bq->count;
}

int bq_isEmpty(BQueue *bq) {
    return (bq->count == 0L);
}

static void **toArray(BQueue *bq) {
    void **tmp = NULL;

    if (bq->count > 0L) {
        tmp = (void **)malloc(bq->count * sizeof(void *));
        if (tmp != NULL) {
            int i, j, n;

            n = bq->count;
            for (i = bq->out, j = 0; n > 0; i = (i+1) % bq->size, j++, n--) {
                tmp[j] = bq->buffer[i];
            }
        }
    }
    return tmp;
}

void **bq_toArray(BQueue *bq, long *len) {
    void **tmp = toArray(bq);

    if (tmp != NULL)
        *len = bq->count;
    return tmp;
}

Iterator *bq_it_create(BQueue *bq) {
    Iterator *it = NULL;
    void **tmp = toArray(bq);

    if (tmp != NULL) {
        it = it_create(bq->count, tmp);
        if (it == NULL)
            free(tmp);
    }
    return it;
}
