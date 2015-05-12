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

#include "tshashmap.h"
#include "hashmap.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define LOCK(hm) &((hm)->lock)

struct tshashmap {
    HashMap *hm;
    pthread_mutex_t lock;	/* this is a recursive lock */
};

TSHashMap *tshm_create(long capacity, double loadFactor) {
    TSHashMap *tshm = (TSHashMap *)malloc(sizeof(TSHashMap));

    if (tshm != NULL) {
        HashMap *hm = hm_create(capacity, loadFactor);

        if (hm == NULL) {
            free(tshm);
            tshm = NULL;
        } else {
            pthread_mutexattr_t ma;
            pthread_mutexattr_init(&ma);
            pthread_mutexattr_settype(&ma, PTHREAD_MUTEX_RECURSIVE);
            tshm->hm = hm;
            pthread_mutex_init(LOCK(tshm), &ma);
            pthread_mutexattr_destroy(&ma);
        }
    }
    return tshm;
}

void tshm_destroy(TSHashMap *hm, void (*userFunction)(void *element)) {
    pthread_mutex_lock(LOCK(hm));
    hm_destroy(hm->hm, userFunction);
    pthread_mutex_unlock(LOCK(hm));
    pthread_mutex_destroy(LOCK(hm));
    free(hm);
}

void tshm_lock(TSHashMap *hm) {
    pthread_mutex_lock(LOCK(hm));
}

void tshm_unlock(TSHashMap *hm) {
    pthread_mutex_unlock(LOCK(hm));
}

void tshm_clear(TSHashMap *hm, void (*userFunction)(void *element)) {
    pthread_mutex_lock(LOCK(hm));
    hm_clear(hm->hm, userFunction);
    pthread_mutex_unlock(LOCK(hm));
}

int tshm_containsKey(TSHashMap *hm, char *key) {
    int result;
    pthread_mutex_lock(LOCK(hm));
    result = hm_containsKey(hm->hm, key);
    pthread_mutex_unlock(LOCK(hm));
    return  result;
}

HMEntry **tshm_entryArray(TSHashMap *hm, long *len) {
    HMEntry **result;
    pthread_mutex_lock(LOCK(hm));
    result = hm_entryArray(hm->hm, len);
    pthread_mutex_unlock(LOCK(hm));
    return  result;
}

int tshm_get(TSHashMap *hm, char *key, void **element) {
    int result;
    pthread_mutex_lock(LOCK(hm));
    result = hm_get(hm->hm, key, element);
    pthread_mutex_unlock(LOCK(hm));
    return  result;
}

int tshm_isEmpty(TSHashMap *hm) {
    int result;
    pthread_mutex_lock(LOCK(hm));
    result = hm_isEmpty(hm->hm);
    pthread_mutex_unlock(LOCK(hm));
    return  result;
}

char **tshm_keyArray(TSHashMap *hm, long *len) {
    char **result;
    pthread_mutex_lock(LOCK(hm));
    result = hm_keyArray(hm->hm, len);
    pthread_mutex_unlock(LOCK(hm));
    return  result;
}

int tshm_put(TSHashMap *hm, char *key, void *element, void **previous) {
    int result;
    pthread_mutex_lock(LOCK(hm));
    result = hm_put(hm->hm, key, element, previous);
    pthread_mutex_unlock(LOCK(hm));
    return  result;
}

int tshm_remove(TSHashMap *hm, char *key, void **element) {
    int result;
    pthread_mutex_lock(LOCK(hm));
    result = hm_remove(hm->hm, key, element);
    pthread_mutex_unlock(LOCK(hm));
    return  result;
}

long tshm_size(TSHashMap *hm) {
    long result;
    pthread_mutex_lock(LOCK(hm));
    result = hm_size(hm->hm);
    pthread_mutex_unlock(LOCK(hm));
    return  result;
}

TSIterator *tshm_it_create(TSHashMap *hm) {
    TSIterator *it = NULL;
    void **tmp;
    long len;

    pthread_mutex_lock(LOCK(hm));
    tmp = (void **)hm_entryArray(hm->hm, &len);
    if (tmp != NULL) {
        it = tsit_create(LOCK(hm), len, tmp);
        if (it == NULL)
            free(tmp);
    }
    if (it == NULL)
        pthread_mutex_unlock(LOCK(hm));
    return it;
}
