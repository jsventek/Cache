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

#include "tslinkedlist.h"
#include "linkedlist.h"
#include <stdlib.h>
#include <pthread.h>

#define LOCK(ll) &((ll)->lock)

struct tslinkedlist {
    LinkedList *ll;
    pthread_mutex_t lock;	/* this is a recursive lock */
};

TSLinkedList *tsll_create(void) {
    TSLinkedList *tsll = (TSLinkedList *)malloc(sizeof(TSLinkedList));

    if (tsll != NULL) {
        LinkedList *ll = ll_create();

        if (ll == NULL) {
            free(tsll);
            tsll = NULL;
        } else {
            pthread_mutexattr_t ma;
            pthread_mutexattr_init(&ma);
            pthread_mutexattr_settype(&ma, PTHREAD_MUTEX_RECURSIVE);
            tsll->ll = ll;
            pthread_mutex_init(LOCK(tsll), &ma);
            pthread_mutexattr_destroy(&ma);
        }
    }
    return tsll;
}

void tsll_destroy(TSLinkedList *tsll, void (*userFunction)(void *element)) {
    pthread_mutex_lock(LOCK(tsll));
    ll_destroy(tsll->ll, userFunction);
    pthread_mutex_unlock(LOCK(tsll));
    pthread_mutex_destroy(LOCK(tsll));
    free(tsll);
}

void tsll_lock(TSLinkedList *tsll) {
    pthread_mutex_lock(LOCK(tsll));
}

void tsll_unlock(TSLinkedList *tsll) {
    pthread_mutex_unlock(LOCK(tsll));
}

int tsll_add(TSLinkedList *tsll, void *element) {
    int result;
    pthread_mutex_lock(LOCK(tsll));
    result = ll_add(tsll->ll, element);
    pthread_mutex_unlock(LOCK(tsll));
    return result;
}

int tsll_insert(TSLinkedList *tsll, long index, void *element) {
    int result;
    pthread_mutex_lock(LOCK(tsll));
    result = ll_insert(tsll->ll, index, element);
    pthread_mutex_unlock(LOCK(tsll));
    return result;
}

int tsll_addFirst(TSLinkedList *tsll, void *element) {
    int result;
    pthread_mutex_lock(LOCK(tsll));
    result = ll_addFirst(tsll->ll, element);
    pthread_mutex_unlock(LOCK(tsll));
    return result;
}

int tsll_addLast(TSLinkedList *tsll, void *element) {
    int result;
    pthread_mutex_lock(LOCK(tsll));
    result = ll_addLast(tsll->ll, element);
    pthread_mutex_unlock(LOCK(tsll));
    return result;
}

void tsll_clear(TSLinkedList *tsll, void (*userFunction)(void *element)) {
    pthread_mutex_lock(LOCK(tsll));
    ll_clear(tsll->ll, userFunction);
    pthread_mutex_unlock(LOCK(tsll));
}

int tsll_get(TSLinkedList *tsll, long index, void **element) {
    int result;
    pthread_mutex_lock(LOCK(tsll));
    result = ll_get(tsll->ll, index, element);
    pthread_mutex_unlock(LOCK(tsll));
    return result;
}

int tsll_getFirst(TSLinkedList *tsll, void **element) {
    int result;
    pthread_mutex_lock(LOCK(tsll));
    result = ll_getFirst(tsll->ll, element);
    pthread_mutex_unlock(LOCK(tsll));
    return result;
}

int tsll_getLast(TSLinkedList *tsll, void **element) {
    int result;
    pthread_mutex_lock(LOCK(tsll));
    result = ll_getLast(tsll->ll, element);
    pthread_mutex_unlock(LOCK(tsll));
    return result;
}

int tsll_remove(TSLinkedList *tsll, long index, void **element) {
    int result;
    pthread_mutex_lock(LOCK(tsll));
    result = ll_remove(tsll->ll, index, element);
    pthread_mutex_unlock(LOCK(tsll));
    return result;
}

int tsll_removeFirst(TSLinkedList *tsll, void **element) {
    int result;
    pthread_mutex_lock(LOCK(tsll));
    result = ll_removeFirst(tsll->ll, element);
    pthread_mutex_unlock(LOCK(tsll));
    return result;
}

int tsll_removeLast(TSLinkedList *tsll, void **element) {
    int result;
    pthread_mutex_lock(LOCK(tsll));
    result = ll_removeLast(tsll->ll, element);
    pthread_mutex_unlock(LOCK(tsll));
    return result;
}

int tsll_set(TSLinkedList *tsll, long index, void *element, void **previous) {
    int result;
    pthread_mutex_lock(LOCK(tsll));
    result = ll_set(tsll->ll, index, element, previous);
    pthread_mutex_unlock(LOCK(tsll));
    return result;
}

long tsll_size(TSLinkedList *tsll) {
    long result;
    pthread_mutex_lock(LOCK(tsll));
    result = ll_size(tsll->ll);
    pthread_mutex_unlock(LOCK(tsll));
    return result;
}

void **tsll_toArray(TSLinkedList *tsll, long *len) {
    void **result;
    pthread_mutex_lock(LOCK(tsll));
    result = ll_toArray(tsll->ll, len);
    pthread_mutex_unlock(LOCK(tsll));
    return result;
}

TSIterator *tsll_it_create(TSLinkedList *tsll) {
    TSIterator *it = NULL;
    void **tmp;
    long len;

    pthread_mutex_lock(LOCK(tsll));
    tmp = ll_toArray(tsll->ll, &len);
    if (tmp != NULL) {
        it = tsit_create(LOCK(tsll), len, tmp);
        if (it == NULL)
            free(tmp);
    }
    if (it == NULL)
        pthread_mutex_unlock(LOCK(tsll));
    return it;
}
