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
 * implementation for thread-safe generic bounded queue
 */

#include "tsbqueue.h"
#include "bqueue.h"
#include <stdlib.h>
#include <pthread.h>

#define LOCK(bq) &((bq)->lock)
#define COND(bq) &((bq)->cond)

struct tsbqueue {
    long cap;
    BQueue *bq;
    pthread_mutex_t lock;	/* this is a recursive lock */
    pthread_cond_t cond;        /* needed for take */
};

TSBQueue *tsbq_create(long capacity) {
    TSBQueue *tsbq = (TSBQueue *)malloc(sizeof(TSBQueue));

    if (tsbq != NULL) {
        BQueue *bq = bq_create(capacity);

        if (bq == NULL) {
            free(tsbq);
            tsbq = NULL;
        } else {
            pthread_mutexattr_t ma;
            long cap = capacity;
            if (cap <= 0L)
                cap = DEFAULT_CAPACITY;
            else if (cap > MAX_CAPACITY)
                cap = MAX_CAPACITY;
            tsbq->cap = cap;
            pthread_mutexattr_init(&ma);
            pthread_mutexattr_settype(&ma, PTHREAD_MUTEX_RECURSIVE);
            tsbq->bq = bq;
            pthread_mutex_init(LOCK(tsbq), &ma);
            pthread_mutexattr_destroy(&ma);
            pthread_cond_init(COND(tsbq), NULL);
        }
    }
    return tsbq;
}

void tsbq_destroy(TSBQueue *tsbq, void (*userFunction)(void *element)) {
    pthread_mutex_lock(LOCK(tsbq));
    bq_destroy(tsbq->bq, userFunction);
    pthread_mutex_unlock(LOCK(tsbq));
    pthread_mutex_destroy(LOCK(tsbq));
    pthread_cond_destroy(COND(tsbq));
    free(tsbq);
}

void tsbq_clear(TSBQueue *tsbq, void (*userFunction)(void *element)) {
    pthread_mutex_lock(LOCK(tsbq));
    bq_clear(tsbq->bq, userFunction);
    pthread_mutex_unlock(LOCK(tsbq));
}

void tsbq_lock(TSBQueue *tsbq) {
    pthread_mutex_lock(LOCK(tsbq));
}

void tsbq_unlock(TSBQueue *tsbq) {
    pthread_mutex_unlock(LOCK(tsbq));
}

int tsbq_add(TSBQueue *tsbq, void *element) {
    int result;
    pthread_mutex_lock(LOCK(tsbq));
    result = bq_add(tsbq->bq, element);
    if (result)
        pthread_cond_signal(COND(tsbq));
    pthread_mutex_unlock(LOCK(tsbq));
    return result;
}

void tsbq_put(TSBQueue *tsbq, void *element) {
    pthread_mutex_lock(LOCK(tsbq));
    while (bq_size(tsbq->bq) == tsbq->cap)
        pthread_cond_wait(COND(tsbq), LOCK(tsbq));
    (void)bq_add(tsbq->bq, element);
    pthread_cond_signal(COND(tsbq));
    pthread_mutex_unlock(LOCK(tsbq));
}

int tsbq_peek(TSBQueue *tsbq, void **element) {
    int result;
    pthread_mutex_lock(LOCK(tsbq));
    result = bq_peek(tsbq->bq, element);
    pthread_mutex_unlock(LOCK(tsbq));
    return result;
}

int tsbq_remove(TSBQueue *tsbq, void **element) {
    int result;
    pthread_mutex_lock(LOCK(tsbq));
    result = bq_remove(tsbq->bq, element);
    if (result)
        pthread_cond_signal(COND(tsbq));
    pthread_mutex_unlock(LOCK(tsbq));
    return result;
}

void tsbq_take(TSBQueue *tsbq, void **element) {
    pthread_mutex_lock(LOCK(tsbq));
    while (bq_size(tsbq->bq) == 0L)
        pthread_cond_wait(COND(tsbq), LOCK(tsbq));
    (void)bq_remove(tsbq->bq, element);
    pthread_cond_signal(COND(tsbq));
    pthread_mutex_unlock(LOCK(tsbq));
}

long tsbq_size(TSBQueue *tsbq) {
    long result;
    pthread_mutex_lock(LOCK(tsbq));
    result = bq_size(tsbq->bq);
    pthread_mutex_unlock(LOCK(tsbq));
    return result;
}

int tsbq_isEmpty(TSBQueue *tsbq) {
    int result;
    pthread_mutex_lock(LOCK(tsbq));
    result = bq_isEmpty(tsbq->bq);
    pthread_mutex_unlock(LOCK(tsbq));
    return result;
}

void **tsbq_toArray(TSBQueue *tsbq, long *len) {
    void **result;
    pthread_mutex_lock(LOCK(tsbq));
    result = bq_toArray(tsbq->bq, len);
    pthread_mutex_unlock(LOCK(tsbq));
    return result;
}

TSIterator *tsbq_it_create(TSBQueue *tsbq) {
    TSIterator *it = NULL;
    void **tmp;
    long len;

    pthread_mutex_lock(LOCK(tsbq));
    tmp = bq_toArray(tsbq->bq, &len);
    if (tmp != NULL) {
        it = tsit_create(LOCK(tsbq), len, tmp);
        if (it == NULL)
            free(tmp);
    }
    if (it == NULL)
        pthread_mutex_unlock(LOCK(tsbq));
    return it;
}
