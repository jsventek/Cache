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
 * implementation for thread-safe generic linked list
 */

#include "tsuqueue.h"
#include "uqueue.h"
#include <stdlib.h>
#include <pthread.h>

#define LOCK(uq) &((uq)->lock)
#define COND(uq) &((uq)->cond)

struct tsuqueue {
    UQueue *uq;
    pthread_mutex_t lock;	/* this is a recursive lock */
    pthread_cond_t cond;        /* needed for take */
};

TSUQueue *tsuq_create(void) {
    TSUQueue *tsuq = (TSUQueue *)malloc(sizeof(TSUQueue));

    if (tsuq != NULL) {
        UQueue *uq = uq_create();

        if (uq == NULL) {
            free(tsuq);
            tsuq = NULL;
        } else {
            pthread_mutexattr_t ma;
            pthread_mutexattr_init(&ma);
            pthread_mutexattr_settype(&ma, PTHREAD_MUTEX_RECURSIVE);
            tsuq->uq = uq;
            pthread_mutex_init(LOCK(tsuq), &ma);
            pthread_mutexattr_destroy(&ma);
            pthread_cond_init(COND(tsuq), NULL);
        }
    }
    return tsuq;
}

void tsuq_destroy(TSUQueue *tsuq, void (*userFunction)(void *element)) {
    pthread_mutex_lock(LOCK(tsuq));
    uq_destroy(tsuq->uq, userFunction);
    pthread_mutex_unlock(LOCK(tsuq));
    pthread_mutex_destroy(LOCK(tsuq));
    pthread_cond_destroy(COND(tsuq));
    free(tsuq);
}

void tsuq_clear(TSUQueue *tsuq, void (*userFunction)(void *element)) {
    pthread_mutex_lock(LOCK(tsuq));
    uq_clear(tsuq->uq, userFunction);
    pthread_mutex_unlock(LOCK(tsuq));
}

void tsuq_lock(TSUQueue *tsuq) {
    pthread_mutex_lock(LOCK(tsuq));
}

void tsuq_unlock(TSUQueue *tsuq) {
    pthread_mutex_unlock(LOCK(tsuq));
}

int tsuq_add(TSUQueue *tsuq, void *element) {
    int result;
    pthread_mutex_lock(LOCK(tsuq));
    result = uq_add(tsuq->uq, element);
    pthread_cond_signal(COND(tsuq));
    pthread_mutex_unlock(LOCK(tsuq));
    return result;
}

int tsuq_peek(TSUQueue *tsuq, void **element) {
    int result;
    pthread_mutex_lock(LOCK(tsuq));
    result = uq_peek(tsuq->uq, element);
    pthread_mutex_unlock(LOCK(tsuq));
    return result;
}

int tsuq_remove(TSUQueue *tsuq, void **element) {
    int result;
    pthread_mutex_lock(LOCK(tsuq));
    result = uq_remove(tsuq->uq, element);
    pthread_mutex_unlock(LOCK(tsuq));
    return result;
}

void tsuq_take(TSUQueue *tsuq, void **element) {
    pthread_mutex_lock(LOCK(tsuq));
    while (uq_size(tsuq->uq) == 0L)
        pthread_cond_wait(COND(tsuq), LOCK(tsuq));
    (void)uq_remove(tsuq->uq, element);
    pthread_mutex_unlock(LOCK(tsuq));
}

long tsuq_size(TSUQueue *tsuq) {
    long result;
    pthread_mutex_lock(LOCK(tsuq));
    result = uq_size(tsuq->uq);
    pthread_mutex_unlock(LOCK(tsuq));
    return result;
}

int tsuq_isEmpty(TSUQueue *tsuq) {
    int result;
    pthread_mutex_lock(LOCK(tsuq));
    result = uq_isEmpty(tsuq->uq);
    pthread_mutex_unlock(LOCK(tsuq));
    return result;
}

void **tsuq_toArray(TSUQueue *tsuq, long *len) {
    void **result;
    pthread_mutex_lock(LOCK(tsuq));
    result = uq_toArray(tsuq->uq, len);
    pthread_mutex_unlock(LOCK(tsuq));
    return result;
}

TSIterator *tsuq_it_create(TSUQueue *tsuq) {
    TSIterator *it = NULL;
    void **tmp;
    long len;

    pthread_mutex_lock(LOCK(tsuq));
    tmp = uq_toArray(tsuq->uq, &len);
    if (tmp != NULL) {
        it = tsit_create(LOCK(tsuq), len, tmp);
        if (it == NULL)
            free(tmp);
    }
    if (it == NULL)
        pthread_mutex_unlock(LOCK(tsuq));
    return it;
}
