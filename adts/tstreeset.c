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

#include "tstreeset.h"
#include "treeset.h"
#include <stdlib.h>
#include <pthread.h>

/*
 * implementation for generic thread-safe treeset implementation
 */

#define LOCK(ts) &((ts)->lock)

struct tstreeset {
    TreeSet *ts;
    pthread_mutex_t lock;
};

TSTreeSet *tsts_create(int (*cmpFunction)(void *, void *)) {
    TSTreeSet *tsts = (TSTreeSet *)malloc(sizeof(TSTreeSet));

    if (tsts != NULL) {
        TreeSet *ts = ts_create(cmpFunction);
        if (ts == NULL) {
            free(tsts);
            tsts = NULL;
        } else {
            pthread_mutexattr_t ma;
            pthread_mutexattr_init(&ma);
            pthread_mutexattr_settype(&ma, PTHREAD_MUTEX_RECURSIVE);
            tsts->ts = ts;
            pthread_mutex_init(LOCK(tsts), &ma);
            pthread_mutexattr_destroy(&ma);
        }
    }
    return tsts;
}

void tsts_destroy(TSTreeSet *ts, void (*userFunction)(void *element)) {
    pthread_mutex_lock(LOCK(ts));
    ts_destroy(ts->ts, userFunction);
    pthread_mutex_unlock(LOCK(ts));
    pthread_mutex_destroy(LOCK(ts));
    free(ts);
}

void tsts_lock(TSTreeSet *ts) {
    pthread_mutex_lock(LOCK(ts));
}

void tsts_unlock(TSTreeSet *ts) {
    pthread_mutex_unlock(LOCK(ts));
}

int tsts_add(TSTreeSet *ts, void *element) {
    int result;
    pthread_mutex_lock(LOCK(ts));
    result = ts_add(ts->ts, element);
    pthread_mutex_unlock(LOCK(ts));
    return result;
}

int tsts_ceiling(TSTreeSet *ts, void *element, void **ceiling) {
    int result;
    pthread_mutex_lock(LOCK(ts));
    result = ts_ceiling(ts->ts, element, ceiling);
    pthread_mutex_unlock(LOCK(ts));
    return result;
}

void tsts_clear(TSTreeSet *ts, void (*userFunction)(void *element)) {
    pthread_mutex_lock(LOCK(ts));
    ts_clear(ts->ts, userFunction);
    pthread_mutex_unlock(LOCK(ts));
}

int tsts_contains(TSTreeSet *ts, void *element) {
    int result;
    pthread_mutex_lock(LOCK(ts));
    result = ts_contains(ts->ts, element);
    pthread_mutex_unlock(LOCK(ts));
    return result;
}

int tsts_first(TSTreeSet *ts, void **element) {
    int result;
    pthread_mutex_lock(LOCK(ts));
    result = ts_first(ts->ts, element);
    pthread_mutex_unlock(LOCK(ts));
    return result;
}

int tsts_floor(TSTreeSet *ts, void *element, void **floor) {
    int result;
    pthread_mutex_lock(LOCK(ts));
    result = ts_floor(ts->ts, element, floor);
    pthread_mutex_unlock(LOCK(ts));
    return result;
}

int tsts_higher(TSTreeSet *ts, void *element, void **higher) {
    int result;
    pthread_mutex_lock(LOCK(ts));
    result = ts_higher(ts->ts, element, higher);
    pthread_mutex_unlock(LOCK(ts));
    return result;
}

int tsts_isEmpty(TSTreeSet *ts) {
    int result;
    pthread_mutex_lock(LOCK(ts));
    result = ts_isEmpty(ts->ts);
    pthread_mutex_unlock(LOCK(ts));
    return result;
}

int tsts_last(TSTreeSet *ts, void **element) {
    int result;
    pthread_mutex_lock(LOCK(ts));
    result = ts_last(ts->ts, element);
    pthread_mutex_unlock(LOCK(ts));
    return result;
}

int tsts_lower(TSTreeSet *ts, void *element, void **lower) {
    int result;
    pthread_mutex_lock(LOCK(ts));
    result = ts_lower(ts->ts, element, lower);
    pthread_mutex_unlock(LOCK(ts));
    return result;
}

int tsts_pollFirst(TSTreeSet *ts, void **element) {
    int result;
    pthread_mutex_lock(LOCK(ts));
    result = ts_pollFirst(ts->ts, element);
    pthread_mutex_unlock(LOCK(ts));
    return result;
}

int tsts_pollLast(TSTreeSet *ts, void **element) {
    int result;
    pthread_mutex_lock(LOCK(ts));
    result = ts_pollLast(ts->ts, element);
    pthread_mutex_unlock(LOCK(ts));
    return result;
}

int tsts_remove(TSTreeSet *ts, void *element, void (*userFunction)(void *element)) {
    int result;
    pthread_mutex_lock(LOCK(ts));
    result = ts_remove(ts->ts, element, userFunction);
    pthread_mutex_unlock(LOCK(ts));
    return result;
}

long tsts_size(TSTreeSet *ts) {
    long result;
    pthread_mutex_lock(LOCK(ts));
    result = ts_size(ts->ts);
    pthread_mutex_unlock(LOCK(ts));
    return result;
}

void **tsts_toArray(TSTreeSet *ts, long *len) {
    void **result;
    pthread_mutex_lock(LOCK(ts));
    result = ts_toArray(ts->ts, len);
    pthread_mutex_unlock(LOCK(ts));
    return result;
}

TSIterator *tsts_it_create(TSTreeSet *ts) {
    TSIterator *it = NULL;
    void **tmp;
    long len;

    pthread_mutex_lock(LOCK(ts));
    tmp = ts_toArray(ts->ts, &len);
    if (tmp != NULL) {
        it = tsit_create(LOCK(ts), len, tmp);
        if (it == NULL)
            free(tmp);
    }
    if (it == NULL)
        pthread_mutex_unlock(LOCK(ts));
    return it;
}
