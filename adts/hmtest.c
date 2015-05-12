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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    char buf[1024];
    char key[20];
    char *p;
    HashMap *hm;
    long i, n;
    FILE *fd;
    HMEntry **array;
    Iterator *it;

    if (argc != 2) {
        fprintf(stderr, "usage: ./hmtest file\n");
        return -1;
    }
    if ((hm = hm_create(0L, 0.0)) == NULL) {
        fprintf(stderr, "Error creating hashmap of strings\n");
        return -1;
    }
    if ((fd = fopen(argv[1], "r")) == NULL) {
        fprintf(stderr, "Unable to open %s to read\n", argv[1]);
        return -1;
    }
    /*
     * test of put()
     */
    printf("===== test of put when key not in hashmap\n");
    i = 0;
    while (fgets(buf, 1024, fd) != NULL) {
        char *prev;

        if ((p = strdup(buf)) == NULL) {
            fprintf(stderr, "Error duplicating string\n");
            return -1;
        }
        sprintf(key, "%ld", i++);
        if (!hm_put(hm, key, p, (void**)&prev)) {
            fprintf(stderr, "Error adding key,string to hashmap\n");
            return -1;
        }
    }
    fclose(fd);
    n = hm_size(hm);
    /*
     * test of get()
     */
    printf("===== test of get\n");
    for (i = 0; i < n; i++) {
        char *element;

        sprintf(key, "%ld", i);
        if (!hm_get(hm, key, (void **)&element)) {
            fprintf(stderr, "Error retrieving %ld'th element\n", i);
            return -1;
        }
        printf("%s,%s", key, element);
    }
    /*
     * test of remove
     */
    printf("===== test of remove\n");
    printf("Size before remove = %ld\n", n);
    for (i = n - 1; i >= 0; i--) {
        sprintf(key, "%ld", i);
        if (!hm_remove(hm, key, (void **)&p)) {
            fprintf(stderr, "Error removing %ld'th element\n", i);
            return -1;
        }
        free(p);
    }
    printf("Size after remove = %ld\n", hm_size(hm));
    /*
     * test of destroy with NULL userFunction
     */
    printf("===== test of destroy(NULL)\n");
    hm_destroy(hm, NULL);
    /*
     * test of insert
     */
    if ((hm = hm_create(0L, 3.0)) == NULL) {
        fprintf(stderr, "Error creating hashmap of strings\n");
        return -1;
    }
    fd = fopen(argv[1], "r");		/* we know we can open it */
    i = 0L;
    while (fgets(buf, 1024, fd) != NULL) {
        char *prev;

        if ((p = strdup(buf)) == NULL) {
            fprintf(stderr, "Error duplicating string\n");
            return -1;
        }
        sprintf(key, "%ld", i++);
        if (!hm_put(hm, key, p, (void **)&prev)) {
            fprintf(stderr, "Error adding key,value to hashmap\n");
            return -1;
        }
    }
    fclose(fd);
    /*
     * test of put replacing value associated with an existing key
     */
    printf("===== test of put (replace value associated with key)\n");
    for (i = 0; i < n; i++) {
        char bf[1024], *q;
        sprintf(bf, "line %ld\n", i);
        if ((p = strdup(bf)) == NULL) {
            fprintf(stderr, "Error duplicating string\n");
            return -1;
        }
        sprintf(key, "%ld", i);
        if (!hm_put(hm, key, p, (void **)&q)) {
            fprintf(stderr, "Error replacing %ld'th element\n", i);
            return -1;
        }
        free(q);
    }
    for (i = 0; i < n; i++) {
        char *element;

        sprintf(key, "%ld", i);
        if (!hm_get(hm, key, (void **)&element)) {
            fprintf(stderr, "Error retrieving %ld'th element\n", i);
            return -1;
        }
        printf("%s,%s", key, element);
    }
    /*
     * test of entryArray
     */
    printf("===== test of entryArray\n");
    if ((array = (HMEntry **)hm_entryArray(hm, &n)) == NULL) {
        fprintf(stderr, "Error in invoking hm_entryArray()\n");
        return -1;
    }
    for (i = 0; i < n; i++) {
        printf("%s,%s", hmentry_key(array[i]), (char *)hmentry_value(array[i]));
    }
    free(array);
    /*
     * test of iterator
     */
    printf("===== test of iterator\n");
    if ((it = hm_it_create(hm)) == NULL) {
        fprintf(stderr, "Error in creating iterator\n");
        return -1;
    }
    while (it_hasNext(it)) {
        HMEntry *p;
        (void) it_next(it, (void **)&p);
        printf("%s,%s", hmentry_key(p), (char *)hmentry_value(p));
    }
    it_destroy(it);
    /*
     * test of destroy with free() as userFunction
     */
    printf("===== test of destroy(free)\n");
    hm_destroy(hm, free);

    return 0;
}
