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
 * implementation for generic unbounded FIFO queue
 */

#include "uqueue.h"
#include "linkedlist.h"
#include <stdlib.h>

struct uqueue {
    LinkedList *ll;
};

UQueue *uq_create(void) {
    UQueue *uq = (UQueue *)malloc(sizeof(UQueue));

    if (uq != NULL) {
        LinkedList *ll = ll_create();

        if (ll == NULL) {
            free(uq);
            uq = NULL;
        } else {
            uq->ll = ll;
        }
    }
    return uq;
}

void uq_destroy(UQueue *uq, void (*userFunction)(void *element)) {
    ll_destroy(uq->ll, userFunction);
    free(uq);
}

void uq_clear(UQueue *uq, void (*userFunction)(void *element)) {
    ll_clear(uq->ll, userFunction);
}

int uq_add(UQueue *uq, void *element) {
    int result;
    result = ll_add(uq->ll, element);
    return result;
}

int uq_peek(UQueue *uq, void **element) {
    int result;
    result = ll_getFirst(uq->ll, element);
    return result;
}

int uq_remove(UQueue *uq, void **element) {
    int result;
    result = ll_removeFirst(uq->ll, element);
    return result;
}

long uq_size(UQueue *uq) {
    long result;
    result = ll_size(uq->ll);
    return result;
}

int uq_isEmpty(UQueue *uq) {
    return (ll_size(uq->ll) == 0L);
}

void **uq_toArray(UQueue *uq, long *len) {
    void **result;
    result = ll_toArray(uq->ll, len);
    return result;
}

Iterator *uq_it_create(UQueue *uq) {
    Iterator *it = NULL;
    void **tmp;
    long len;

    tmp = ll_toArray(uq->ll, &len);
    if (tmp != NULL) {
        it = it_create(len, tmp);
        if (it == NULL)
            free(tmp);
    }
    return it;
}
