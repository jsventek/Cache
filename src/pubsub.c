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
 * tables for mapping from id to subscriptions and vice versa
 */
#include "pubsub.h"
#include "hashmap.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define SUBSCRIPTION_BUCKETS 20

static HashMap *id2sub = NULL;
static HashMap *sub2id = NULL;

int ps_create(long id, Subscription *s) {
    char buf[1024], *p;
    void *dummy;

    if (id2sub == NULL) {
        id2sub = hm_create(SUBSCRIPTION_BUCKETS, 0.75);
        sub2id = hm_create(SUBSCRIPTION_BUCKETS, 0.75);
    }
    sprintf(buf, "%08lx", id);
    if (! hm_put(id2sub, buf, (void *)s, &dummy))
        return 0;
    if (! (p = strdup(buf))) {
        hm_remove(id2sub, buf, &dummy);
        return 0;
    }
    sprintf(buf, "%s!%s!%s!%s", s->queryname, s->ipaddr, s->port, s->service);
    if (! hm_put(sub2id, buf, p, &dummy)) {
        (void)hm_remove(id2sub, p, &dummy);
        free(p);
        return 0;
    }
    return 1;
}

void ps_delete(long id) {
    char buf[1024];
    Subscription *s;

    sprintf(buf, "%08lx", id);
    if (hm_get(id2sub, buf, (void **)&s)) {
        char *p;
        void *dummy;
        sprintf(buf, "%s!%s!%s!%s", s->queryname, s->ipaddr, s->port, s->service);
        (void)hm_get(sub2id, buf, (void **)&p);
        (void)hm_remove(sub2id, buf, &dummy);
        (void)hm_remove(id2sub, p, &dummy);
        free(p);
    }
}

Subscription *ps_id2sub(long id) {
    char buf[1024];
    Subscription *s;

    sprintf(buf, "%08lx", id);
    if (!hm_get(id2sub, buf, (void **)&s))
        return NULL;
    return s;
}

int ps_sub2id(Subscription *s, long *id) {
    char *p, buf[1024];

    sprintf(buf, "%s!%s!%s!%s", s->queryname, s->ipaddr, s->port, s->service);
    if (hm_get(sub2id, buf, (void **)&p)) {
        *id = strtol(p, (char **)NULL, 16);
        return 1;
    } else {
        return 0;
    }
}
