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

#include "uqueue.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    char buf[1024];
    char *p, *q;
    UQueue *uq;
    long i, n;
    FILE *fd;
    char **array;
    Iterator *it;

    if (argc != 2) {
        fprintf(stderr, "usage: ./uqtest file\n");
        return -1;
    }
    if ((fd = fopen(argv[1], "r")) == NULL) {
        fprintf(stderr, "Unable to open %s to read\n", argv[1]);
        return -1;
    }
    if ((uq = uq_create()) == NULL) {
        fprintf(stderr, "Error creating unbounded queue of strings\n");
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
        if (!uq_add(uq, p)) {
            fprintf(stderr, "Error adding string to bounded queue\n");
            return -1;
        }
    }
    fclose(fd);
    n = uq_size(uq);
    /*
     * test of peek and remove()
     */
    printf("===== test of peek and remove\n");
    for (i = 0; i < n; i++) {
        if (!uq_peek(uq, (void **)&p)) {
            fprintf(stderr, "Error retrieving %ld'th element\n", i);
            return -1;
        }
        if (!uq_remove(uq, (void **)&q)) {
            fprintf(stderr, "Error retrieving %ld'th element\n", i);
            return -1;
        }
        if (strcmp(p, q) != 0) {
            fprintf(stderr, "Returns from peek and remove are not the same\n");
            return -1;
        }
        printf("%s", p);
    }
    /*
     * test of destroy with free userFunction
     */
    printf("===== test of destroy(free)\n");
    uq_destroy(uq, free);
    if ((uq = uq_create()) == NULL) {
        fprintf(stderr, "Error creating bounded queue of strings\n");
        return -1;
    }
    fd = fopen(argv[1], "r");		/* we know we can open it */
    while (fgets(buf, 1024, fd) != NULL) {
        if ((p = strdup(buf)) == NULL) {
            fprintf(stderr, "Error duplicating string\n");
            return -1;
        }
        if (!uq_add(uq, p)) {
            fprintf(stderr, "Error adding string to bounded queue\n");
            return -1;
        }
    }
    fclose(fd);
    /*
     * test of toArray
     */
    printf("===== test of toArray\n");
    if ((array = (char **)uq_toArray(uq, &n)) == NULL) {
        fprintf(stderr, "Error in invoking uq_toArray()\n");
        return -1;
    }
    for (i = 0; i < n; i++) {
        printf("%s", array[i]);
    }
    free(array);
    /*
     * test of iterator
     */
    printf("===== test of iterator\n");
    if ((it = uq_it_create(uq)) == NULL) {
        fprintf(stderr, "Error in creating iterator\n");
        return -1;
    }
    while (it_hasNext(it)) {
        char *p;
        (void) it_next(it, (void **)&p);
        printf("%s", p);
    }
    it_destroy(it);
    /*
     * test of uq_clear(free)
     */
    printf("===== test of uq_clear(free)\n");
    uq_clear(uq, free);
    printf("===== test of uq_destroy(NULL)\n");
    uq_destroy(uq, NULL);

    return 0;
}
