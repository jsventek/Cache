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
