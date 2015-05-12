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

#include "hashmap.h"
#include <stdlib.h>
#include <string.h>

#define DEFAULT_CAPACITY 16
#define MAX_CAPACITY 134217728L
#define DEFAULT_LOAD_FACTOR 0.75
#define TRIGGER 100	/* number of changes that will trigger a load check */

struct hashmap {
    long size;
    long capacity;
    long changes;
    double load;
    double loadFactor;
    double increment;
    HMEntry **buckets;
};

struct hmentry {
    struct hmentry *next;
    char *key;
    void *element;
};

/*
 * generate hash value from key; value returned in range of 0..N-1
 */
#define SHIFT 7L			/* should be prime */
static long hash(char *key, long N) {
    long ans = 0L;
    char *sp;

    for (sp = key; *sp != '\0'; sp++)
        ans = ((SHIFT * ans) + *sp) % N;
    return ans;
}

HashMap *hm_create(long capacity, double loadFactor) {
    HashMap *hm;
    long N;
    double lf;
    HMEntry **array;
    long i;

    hm = (HashMap *)malloc(sizeof(HashMap));
    if (hm != NULL) {
        N = ((capacity > 0) ? capacity : DEFAULT_CAPACITY);
        if (N > MAX_CAPACITY)
            N = MAX_CAPACITY;
        lf = ((loadFactor > 0.000001) ? loadFactor : DEFAULT_LOAD_FACTOR);
        array = (HMEntry **)malloc(N * sizeof(HMEntry *));
        if (array != NULL) {
            hm->capacity = N;
            hm->loadFactor = lf;
            hm->size = 0L;
            hm->load = 0.0;
            hm->changes = 0L;
            hm->increment = 1.0 / (double)N;
            hm->buckets = array;
            for (i = 0; i < N; i++)
                array[i] = NULL;
        } else {
            free(hm);
            hm = NULL;
        }
    }
    return hm;
}

/*
 * traverses the hashmap, calling userFunction on each element
 * then frees storage associated with the key and the HMEntry structure
 */
static void purge(HashMap *hm, void (*userFunction)(void *element)) {

    long i;

    for (i = 0L; i < hm->capacity; i++) {
        HMEntry *p, *q;
        p = hm->buckets[i];
        while (p != NULL) {
            if (userFunction != NULL)
                (*userFunction)(p->element);
            q = p->next;
            free(p->key);
            free(p);
            p = q;
        }
        hm->buckets[i] = NULL;
    }
}

void hm_destroy(HashMap *hm, void (*userFunction)(void *element)) {
    purge(hm, userFunction);
    free(hm->buckets);
    free(hm);
}

void hm_clear(HashMap *hm, void (*userFunction)(void *element)) {
    purge(hm, userFunction);
    hm->size = 0;
    hm->load = 0.0;
    hm->changes = 0;
}

/*
 * local function to locate key in a hashmap
 *
 * returns pointer to entry, if found, as function value; NULL if not found
 * returns bucket index in `bucket'
 */
static HMEntry *findKey(HashMap *hm, char *key, long *bucket) {
    long i = hash(key, hm->capacity);
    HMEntry *p;

    *bucket = i;
    for (p = hm->buckets[i]; p != NULL; p = p->next) {
        if (strcmp(p->key, key) == 0) {
            break;
        }
    }
    return p;
}

int hm_containsKey(HashMap *hm, char *key) {
    long bucket;

    return (findKey(hm, key, &bucket) != NULL);
}

/*
 * local function for generating an array of HMEntry * from a hashmap
 *
 * returns pointer to the array or NULL if malloc failure
 */
static HMEntry **entries(HashMap *hm) {
    HMEntry **tmp = NULL;
    if (hm->size > 0L) {
        size_t nbytes = hm->size * sizeof(HMEntry *);
        tmp = (HMEntry **)malloc(nbytes);
        if (tmp != NULL) {
            long i, n = 0L;
            for (i = 0L; i < hm->capacity; i++) {
                HMEntry *p;
                p = hm->buckets[i];
                while (p != NULL) {
                    tmp[n++] = p;
                    p = p->next;
                }
            }
        }
    }
    return tmp;
}

HMEntry **hm_entryArray(HashMap *hm, long *len) {
    HMEntry **tmp = entries(hm);

    if (tmp != NULL)
        *len = hm->size;
    return tmp;
}

int hm_get(HashMap *hm, char *key, void **element) {
    long i;
    HMEntry *p;
    int ans = 0;

    p = findKey(hm, key, &i);
    if (p != NULL) {
        ans = 1;
        *element = p->element;
    }
    return ans;
}

int hm_isEmpty(HashMap *hm) {
    return (hm->size == 0L);
}

/*
 * local function for generating an array of keys from a hashmap
 *
 * returns pointer to the array or NULL if malloc failure
 */
static char **keys(HashMap *hm) {
    char **tmp = NULL;
    if (hm->size > 0L) {
        size_t nbytes = hm->size * sizeof(char *);
        tmp = (char **)malloc(nbytes);
        if (tmp != NULL) {
            long i, n = 0L;
            for (i = 0L; i < hm->capacity; i++) {
                HMEntry *p;
                p = hm->buckets[i];
                while (p != NULL) {
                    tmp[n++] = p->key;
                    p = p->next;
                }
            }
        }
    }
    return tmp;
}

char **hm_keyArray(HashMap *hm, long *len) {
    char **tmp = keys(hm);

    if (tmp != NULL)
        *len = hm->size;
    return tmp;
}

/*
 * routine that resizes the hashmap
 */
void resize(HashMap *hm) {
    int N;
    HMEntry *p, *q, **array;
    long i, j;

    N = 2 * hm->capacity;
    if (N > MAX_CAPACITY)
        N = MAX_CAPACITY;
    array = (HMEntry **)malloc(N * sizeof(HMEntry *));
    if (array == NULL)
        return;
    for (j = 0; j < N; j++)
        array[j] = NULL;
    /*
     * now redistribute the entries into the new set of buckets
     */
    for (i = 0; i < hm->capacity; i++) {
        for (p = hm->buckets[i]; p != NULL; p = q) {
            q = p->next;
            j = hash(p->key, N);
            p->next = array[j];
            array[j] = p;
        }
    }
    free(hm->buckets);
    hm->buckets = array;
    hm->capacity = N;
    hm->load /= 2.0;
    hm->changes = 0;
    hm->increment = 1.0 / (double)N;
}

int hm_put(HashMap *hm, char *key, void *element, void **previous) {
    long i;
    HMEntry *p;
    int ans = 0;

    if (hm->changes > TRIGGER) {
        hm->changes = 0;
        if (hm->load > hm->loadFactor)
            resize(hm);
    }
    p = findKey(hm, key, &i);
    if (p != NULL) {
        *previous = p->element;
        p->element = element;
        ans = 1;
    } else {
        p = (HMEntry *)malloc(sizeof(HMEntry));
        if (p != NULL) {
            char *q = strdup(key);
            if (q != NULL) {
                p->key = q;
                p->element = element;
                p->next = hm->buckets[i];
                hm->buckets[i] = p;
                *previous = NULL;
                hm->size++;
                hm->load += hm->increment;
                hm->changes++;
                ans = 1;
            } else {
                free(p);
            }
        }
    }
    return ans;
}

int hm_remove(HashMap *hm, char *key, void **element) {
    long i;
    HMEntry *entry;
    int ans = 0;

    entry = findKey(hm, key, &i);
    if (entry != NULL) {
        HMEntry *p, *c;
        *element = entry->element;
        /* determine where the entry lives in the singly linked list */
        for (p = NULL, c = hm->buckets[i]; c != entry; p = c, c = c->next)
            ;
        if (p == NULL)
            hm->buckets[i] = entry->next;
        else
            p->next = entry->next;
        hm->size--;
        hm->load -= hm->increment;
        hm->changes++;
        free(entry->key);
        free(entry);
        ans = 1;
    }
    return ans;
}

long hm_size(HashMap *hm) {
    return hm->size;
}

Iterator *hm_it_create(HashMap *hm) {
    Iterator *it = NULL;
    void **tmp = (void **)entries(hm);

    if (tmp != NULL) {
        it = it_create(hm->size, tmp);
        if (it == NULL)
            free(tmp);
    }
    return it;
}

char *hmentry_key(HMEntry *hme) {
    return hme->key;
}

void *hmentry_value(HMEntry *hme) {
    return hme->element;
}
