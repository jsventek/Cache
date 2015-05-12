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
 * implementation for generic array list
 */

#include "arraylist.h"
#include <stdlib.h>

#define DEFAULT_CAPACITY 10L

struct arraylist {
    long capacity;
    long size;
    void **theArray;
};

ArrayList *al_create(long capacity) {
    ArrayList *al = (ArrayList *)malloc(sizeof(ArrayList));

    if (al != NULL) {
        long cap;
        void **array = NULL;

        cap = (capacity <= 0) ? DEFAULT_CAPACITY : capacity;
        array = (void **) malloc(cap * sizeof(void *));
        if (array == NULL) {
            free(al);
            al = NULL;
        } else {
            al->capacity = cap;
            al->size = 0L;
            al->theArray = array;
        }
    }
    return al;
}

/*
 * traverses arraylist, calling userFunction on each element
 */
static void purge(ArrayList *al, void (*userFunction)(void *element)) {

    if (userFunction != NULL) {
        long i;

        for (i = 0L; i < al->size; i++) {
            (*userFunction)(al->theArray[i]); /* user frees element storage */
            al->theArray[i] = NULL;
        }
    }
}

void al_destroy(ArrayList *al, void (*userFunction)(void *element)) {
    purge(al, userFunction);
    free(al->theArray);			  /* we free array of pointers */
    free(al);				  /* we free the ArrayList struct */
}

int al_add(ArrayList *al, void *element) {
    int status = 1;

    if (al->capacity <= al->size) {	/* need to reallocate */
        size_t nbytes = 2 * al->capacity * sizeof(void *);
        void **tmp = (void **)realloc(al->theArray, nbytes);
        if (tmp == NULL)
            status = 0;	/* allocation failure */
        else {
            al->theArray = tmp;
            al->capacity *= 2;
        }
    }
    if (status)
        al->theArray[al->size++] = element;
    return status;
}

void al_clear(ArrayList *al, void (*userFunction)(void *element)) {
    purge(al, userFunction);
    al->size = 0L;
}

int al_ensureCapacity(ArrayList *al, long minCapacity) {
    int status = 1;

    if (al->capacity < minCapacity) {	/* must extend */
        void **tmp = (void **)realloc(al->theArray, minCapacity * sizeof(void *));
        if (tmp == NULL)
            status = 0;	/* allocation failure */
        else {
            al->theArray = tmp;
            al->capacity = minCapacity;
        }
    }
    return status;
}

int al_get(ArrayList *al, long i, void **element) {
    int status = 0;

    if (i >= 0L && i < al->size) {
        *element = al->theArray[i];
        status = 1;
    }
    return status;
}

int al_insert(ArrayList *al, long i, void *element) {
    int status = 1;

    if (i > al->size)
        return 0;				/* 0 <= i <= size */
    if (al->capacity <= al->size) {	/* need to reallocate */
        size_t nbytes = 2 * al->capacity * sizeof(void *);
        void **tmp = (void **)realloc(al->theArray, nbytes);
        if (tmp == NULL)
            status = 0;	/* allocation failure */
        else {
            al->theArray = tmp;
            al->capacity *= 2;
        }
    }
    if (status) {
        long j;
        for (j = al->size; j > i; j--)		/* slide items up */
            al->theArray[j] = al->theArray[j-1];
        al->theArray[i] = element;
        al->size++;
    }
    return status;
}

int al_isEmpty(ArrayList *al) {
    return (al->size == 0L);
}

int al_remove(ArrayList *al, long i, void **element) {
    int status = 0;
    long j;

    if (i >= 0L && i < al->size) {
        *element = al->theArray[i];
        for (j = i + 1; j < al->size; j++)
            al->theArray[i++] = al->theArray[j];
        al->size--;
        status = 1;
    }
    return status;
}

int al_set(ArrayList *al, void *element, long i, void **previous) {
    int status = 0;

    if (i >= 0L && i < al->size) {
        *previous = al->theArray[i];
        al->theArray[i] = element;
        status = 1;
    }
    return status;
}

long al_size(ArrayList *al) {
    return al->size;
}

/*
 * local function that duplicates the array of void * pointers on the heap
 *
 * returns pointer to duplicate array or NULL if malloc failure
 */
static void **arraydupl(ArrayList *al) {
    void **tmp = NULL;
    if (al->size > 0L) {
        size_t nbytes = al->size * sizeof(void *);
        tmp = (void **)malloc(nbytes);
        if (tmp != NULL) {
            long i;

            for (i = 0; i < al->size; i++)
                tmp[i] = al->theArray[i];
        }
    }
    return tmp;
}

void **al_toArray(ArrayList *al, long *len) {
    void **tmp = arraydupl(al);

    if (tmp != NULL)
        *len = al->size;
    return tmp;
}

int al_trimToSize(ArrayList *al) {
    int status = 0;

    void **tmp = (void **)realloc(al->theArray, al->size * sizeof(void *));
    if (tmp != NULL) {
        status = 1;
        al->theArray = tmp;
        al->capacity = al->size;
    }
    return status;
}

Iterator *al_it_create(ArrayList *al) {
    Iterator *it = NULL;
    void **tmp = arraydupl(al);

    if (tmp != NULL) {
        it = it_create(al->size, tmp);
        if (it == NULL)
            free(tmp);
    }
    return it;
}
