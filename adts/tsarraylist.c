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

#include "tsarraylist.h"
#include "arraylist.h"
#include <stdlib.h>
#include <pthread.h>

#define LOCK(al) &((al)->lock)

/*
 * implementation for thread-safe generic arraylist implementation
 */

struct tsarraylist {
    ArrayList *al;
    pthread_mutex_t lock;	/* this is a recursive lock */
};

TSArrayList *tsal_create(long capacity) {
    TSArrayList *tsal = (TSArrayList *)malloc(sizeof(TSArrayList));

    if (tsal != NULL) {
        ArrayList *al = al_create(capacity);

        if (al == NULL) {
            free(tsal);
            tsal = NULL;
        } else {
            pthread_mutexattr_t ma;
            pthread_mutexattr_init(&ma);
            pthread_mutexattr_settype(&ma, PTHREAD_MUTEX_RECURSIVE);
            tsal->al = al;
            pthread_mutex_init(LOCK(tsal), &ma);
            pthread_mutexattr_destroy(&ma);
        }
    }
    return tsal;
}

void tsal_destroy(TSArrayList *al, void (*userFunction)(void *element)) {
    pthread_mutex_lock(LOCK(al));
    al_destroy(al->al, userFunction);
    pthread_mutex_unlock(LOCK(al));
    pthread_mutex_destroy(LOCK(al));
    free(al);
}

void tsal_lock(TSArrayList *al) {
    pthread_mutex_lock(LOCK(al));
}

void tsal_unlock(TSArrayList *al) {
    pthread_mutex_unlock(LOCK(al));
}

int tsal_add(TSArrayList *al, void *element) {
    int result;
    pthread_mutex_lock(LOCK(al));
    result = al_add(al->al, element);
    pthread_mutex_unlock(LOCK(al));
    return result;
}

void tsal_clear(TSArrayList *al, void (*userFunction)(void *element)) {
    pthread_mutex_lock(LOCK(al));
    al_clear(al->al, userFunction);
    pthread_mutex_unlock(LOCK(al));
}

int tsal_ensureCapacity(TSArrayList *al, long minCapacity) {
    int result;
    pthread_mutex_lock(LOCK(al));
    result = al_ensureCapacity(al->al, minCapacity);
    pthread_mutex_unlock(LOCK(al));
    return result;
}

int tsal_get(TSArrayList *al, long i, void **element) {
    int result;
    pthread_mutex_lock(LOCK(al));
    result = al_get(al->al, i, element);
    pthread_mutex_unlock(LOCK(al));
    return result;
}

int tsal_insert(TSArrayList *al, long i, void *element) {
    int result;
    pthread_mutex_lock(LOCK(al));
    result = al_insert(al->al, i, element);
    pthread_mutex_unlock(LOCK(al));
    return result;
}

int tsal_isEmpty(TSArrayList *al) {
    int result;
    pthread_mutex_lock(LOCK(al));
    result = al_isEmpty(al->al);
    pthread_mutex_unlock(LOCK(al));
    return result;
}

int tsal_remove(TSArrayList *al, long i, void **element) {
    int result;
    pthread_mutex_lock(LOCK(al));
    result = al_remove(al->al, i, element);
    pthread_mutex_unlock(LOCK(al));
    return result;
}

int tsal_set(TSArrayList *al, void *element, long i, void **previous) {
    int result;
    pthread_mutex_lock(LOCK(al));
    result = al_set(al->al, element, i, previous);
    pthread_mutex_unlock(LOCK(al));
    return result;
}

long tsal_size(TSArrayList *al) {
    long result;
    pthread_mutex_lock(LOCK(al));
    result = al_size(al->al);
    pthread_mutex_unlock(LOCK(al));
    return result;
}

void **tsal_toArray(TSArrayList *al, long *len) {
    void **result;
    pthread_mutex_lock(LOCK(al));
    result = al_toArray(al->al, len);
    pthread_mutex_unlock(LOCK(al));
    return result;
}

int tsal_trimToSize(TSArrayList *al) {
    int result;
    pthread_mutex_lock(LOCK(al));
    result = al_trimToSize(al->al);
    pthread_mutex_unlock(LOCK(al));
    return result;
}

TSIterator *tsal_it_create(TSArrayList *al) {
    TSIterator *it = NULL;
    void **tmp;
    long len;

    pthread_mutex_lock(LOCK(al));
    tmp = al_toArray(al->al, &len);
    if (tmp != NULL) {
        it = tsit_create(LOCK(al), len, tmp);
        if (it == NULL)
            free(tmp);
    }
    if (it == NULL)
        pthread_mutex_unlock(LOCK(al));
    return it;
}
