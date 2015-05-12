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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    char buf[1024];
    char *p;
    TSArrayList *al;
    long i, n;
    FILE *fd;
    char **array;
    TSIterator *it;

    if (argc != 2) {
        fprintf(stderr, "usage: ./altest file\n");
        return -1;
    }
    if ((al = tsal_create(0L)) == NULL) {
        fprintf(stderr, "Error creating array list of strings\n");
        return -1;
    }
    if ((fd = fopen(argv[1], "r")) == NULL) {
        fprintf(stderr, "Unable to open %s to read\n", argv[1]);
        return -1;
    }
    /*
     * test of add()
     */
    printf("===== test of add\n");
    while (fgets(buf, 1024, fd) != NULL) {
        if ((p = strdup(buf)) == NULL) {
            fprintf(stderr, "Error duplicating string\n");
            return -1;
        }
        if (!tsal_add(al, p)) {
            fprintf(stderr, "Error adding string to array list\n");
            return -1;
        }
    }
    fclose(fd);
    n = tsal_size(al);
    /*
     * test of get()
     */
    printf("===== test of get\n");
    for (i = 0; i < n; i++) {
        if (!tsal_get(al, i, (void **)&p)) {
            fprintf(stderr, "Error retrieving %ld'th element\n", i);
            return -1;
        }
        printf("%s", p);
    }
    printf("===== test of remove\n");
    /*
     * test of remove
     */
    for (i = n - 1; i >= 0; i--) {
        if (!tsal_remove(al, i, (void **)&p)) {
            fprintf(stderr, "Error removing string from array list\n");
            return -1;
        }
        free(p);
    }
    printf("===== test of destroy(NULL)\n");
    /*
     * test of destroy with NULL userFunction
     */
    tsal_destroy(al, NULL);
    /*
     * test of insert
     */
    if ((al = tsal_create(0L)) == NULL) {
        fprintf(stderr, "Error creating array list of strings\n");
        return -1;
    }
    fd = fopen(argv[1], "r");		/* we know we can open it */
    printf("===== test of insert\n");
    while (fgets(buf, 1024, fd) != NULL) {
        if ((p = strdup(buf)) == NULL) {
            fprintf(stderr, "Error duplicating string\n");
            return -1;
        }
        if (!tsal_insert(al, 0, p)) {
            fprintf(stderr, "Error adding string to array list\n");
            return -1;
        }
    }
    fclose(fd);
    for (i = 0; i < n; i++) {
        if (!tsal_get(al, i, (void **)&p)) {
            fprintf(stderr, "Error retrieving %ld'th element\n", i);
            return -1;
        }
        printf("%s", p);
    }
    printf("===== test of set\n");
    /*
     * test of set
     */
    for (i = 0; i < n; i++) {
        char bf[1024], *q;
        sprintf(bf, "line %ld\n", i);
        if ((p = strdup(bf)) == NULL) {
            fprintf(stderr, "Error duplicating string\n");
            return -1;
        }
        if (!tsal_set(al, p, i, (void **)&q)) {
            fprintf(stderr, "Error replacing %ld'th element\n", i);
            return -1;
        }
        free(q);
    }
    printf("===== test of toArray\n");
    /*
     * test of toArray
     */
    tsal_lock(al);
    if ((array = (char **)tsal_toArray(al, &n)) == NULL) {
        fprintf(stderr, "Error in invoking tsal_toArray()\n");
        return -1;
    }
    for (i = 0; i < n; i++) {
        printf("%s", array[i]);
    }
    tsal_unlock(al);
    free(array);
    printf("===== test of iterator\n");
    /*
     * test of iterator
     */
    if ((it = tsal_it_create(al)) == NULL) {
        fprintf(stderr, "Error in creating iterator\n");
        return -1;
    }
    while (tsit_hasNext(it)) {
        char *p;
        (void) tsit_next(it, (void **)&p);
        printf("%s", p);
    }
    tsit_destroy(it);
    printf("===== test of destroy(free)\n");
    /*
     * test of destroy with free() as userFunction
     */
    tsal_destroy(al, free);

    return 0;
}
