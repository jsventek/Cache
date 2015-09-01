/*
 * Topic ADT implementation
 */
#include "topic.h"
#include "event.h"
#include "automaton.h"
#include "adts/tshashmap.h"
#include "adts/linkedlist.h"
#include "dataStackEntry.h"
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

#define DEFAULT_STREAM_TABLE_SIZE 20

typedef struct topic {
    int ncells;
    SchemaCell *schema;
    LinkedList *regAUs;
    pthread_mutex_t lock;
} Topic;

static TSHashMap *topicTable;

void top_init(void) {
    topicTable = tshm_create(DEFAULT_STREAM_TABLE_SIZE, 0.75);
}

int top_exist(char *name) {

    return tshm_containsKey(topicTable, name);
}

static SchemaCell *unpack(char *schema, int *ncells) {
    SchemaCell *ans;
    char *p = schema;
    int ncols = 0;

    while (isdigit((int)*p))
        ncols = 10 * ncols + (*p++ - '0');
    *ncells = ncols;
    ans = (SchemaCell *)malloc(ncols * sizeof(SchemaCell));
    if (ans) {
        int i, theType;
        char *name, *type;

        for (i = 0; i < ncols; i++) {
            while (isspace((int)*p))
                p++;
            name = p;
            while (*p != '/')
                p++;
            *p++ = '\0';
            type = p;
            while (isalpha((int)*p))
                p++;
            *p++ = '\0';
            if (strcmp(type, "boolean") == 0)
                theType = dBOOLEAN;
            else if (strcmp(type, "integer") == 0)
                theType = dINTEGER;
            else if (strcmp(type, "real") == 0)
                theType = dDOUBLE;
            else if (strcmp(type, "varchar") == 0)
                theType = dSTRING;
            else if (strcmp(type, "timestamp") == 0)
                theType = dTSTAMP;
            else {
                break;
            }
            ans[i].name = (char *)malloc(strlen(name) + 1);
            strcpy(ans[i].name, name);
            ans[i].type = theType;
        }
        if (i < ncols) {
            int j;
            for (j = 0; j < i; j++)
                free(ans[j].name);
            free(ans);
            ans = NULL;
        }
    }
    return ans;
}

int top_create(char *name, char *schema) {
    Topic *st;
    int ncells;
    char bf[1024];

    if (tshm_containsKey(topicTable, name))	/* topic already defined */
        return 0;
    st = (Topic *)malloc(sizeof(Topic));
    if (st != NULL) {
        void *dummy;
        strcpy(bf, schema);
        pthread_mutex_init(&(st->lock), NULL);
        st->schema = unpack(bf, &ncells);
        if (st->schema != NULL) {
            st->ncells = ncells;
            st->regAUs = ll_create();
            if (st->regAUs != NULL) {
                if (tshm_put(topicTable, name, st, &dummy))
                    return 1;
                ll_destroy(st->regAUs, NULL);
            }
            free((void *)(st->schema));
        }
        free((void *)st);
    }
    return 0;
}

int top_publish(char *name, char *message, RpcEndpoint *ep) {
    int ret = 0;
    Topic *st;

    if (tshm_get(topicTable, name, (void **)&st)) {
        ret = 1;
        pthread_mutex_lock(&(st->lock));
        if (ll_size(st->regAUs) > 0L) {
            Iterator *iter;
            iter = ll_it_create(st->regAUs);
            if (iter) {
                Event *event;
                event = ev_create(name, message, ep, ll_size(st->regAUs));
                if (event) {
                    unsigned long id;
                    while (it_hasNext(iter)) {
                        (void) it_next(iter, (void **)&id);
                        au_publish(id, event);
                    }
                }
                it_destroy(iter);
            }
        }
        pthread_mutex_unlock(&(st->lock));
    }
    return ret;
}

int top_subscribe(char *name, unsigned long id) {
    Topic *st;

    if (tshm_get(topicTable, name, (void **)&st)) {
        pthread_mutex_lock(&(st->lock));
        (void)ll_add(st->regAUs, (void *)id);
        pthread_mutex_unlock(&(st->lock));
        return 1;
    }
    return 0;
}

void top_unsubscribe(char *name, unsigned long id) {
    Topic *st;

    if (tshm_get(topicTable, name, (void **)&st)) {
        long n;
        unsigned long entId;

        /* remove entry that matches au->id */
        pthread_mutex_lock(&(st->lock));
        for(n = 0; n < ll_size(st->regAUs); n++) {
            (void)ll_get(st->regAUs, n, (void **)&entId);
            if (entId == id) {
                (void)ll_remove(st->regAUs, n, (void **)&entId);
                break;
            }
        }
        pthread_mutex_unlock(&(st->lock));
    }
}

int top_schema(char *name, int *ncells, SchemaCell **schema) {
    Topic *st;

    if (tshm_get(topicTable, name, (void **)&st)) {
        *ncells = st->ncells;
        *schema = st->schema;
        return 1;
    }
    return 0;
}

int top_index(char *topic, char *column) {
    Topic *st;

    if (tshm_get(topicTable, topic, (void **)&st)) {
        int i;
        for (i = 0; i < st->ncells; i++)
            if (strcmp(column, st->schema[i].name) == 0)
                return i;
    }
    return -1;
}
