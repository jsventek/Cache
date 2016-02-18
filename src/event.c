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
 * Event ADT implementation
 */
#include "event.h"
#include "dataStackEntry.h"
#include "topic.h"
#include "timestamp.h"
#include "code.h"
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

struct event {
    int refCount, ncols;
    char *data;
    char *topic;
    DataStackEntry *theData;
};

/*
 * `sd' is expected in the following format:
 *
 * 	col 1<|>col 2<|>...<|>col n\n
 *
 * upon return, `d' will have the data converted into DataStackCell's
 *
 * modifies `sd' during processing, replacing the `<' of "<|>" sequences to
 * '\0'; also replaces '\n' with '\0', as well
 *
 * if the type of a column is dSTRING, the DataStackCell will point to the
 * string in situ in `sd'
 */
static void unpack(char *sd, int n, SchemaCell *schema, DataStackEntry *d) {
    char *p, *q;
    int i;

    if ((p = strrchr(sd, '\n')))
        *p = '\0';
    for (i = 0, p = sd; i < n; i++, p = q) {
        if ((q = strstr(p, "<|>"))) {
            *q = '\0';
            q += 3;
        }
        d[i].type = schema[i].type;
        d[i].flags = 0;		      /* no flags set */
        switch(schema[i].type) {
        case dBOOLEAN:
            d[i].value.bool_v = atoi(p);
            break;
        case dINTEGER:
            d[i].value.int_v = atoll(p);
            break;
        case dDOUBLE:
            d[i].value.dbl_v = atof(p);
            break;
        case dSTRING:
            d[i].value.str_v = p;
            d[i].flags |= DUPLICATE;  /* strings will have to be duplicated */
            break;
        case dTSTAMP:
            d[i].value.tstamp_v = string_to_timestamp(p);
            break;
        }
    }
}

Event *ev_create(char *name, char *eventData, unsigned long nAUs) {
    int ncols;
    SchemaCell *schema;
    DataStackEntry *d;

    Event *t = (Event *)malloc(sizeof(Event));
    if (t) {
        t->data = strdup(eventData);
        if (t->data) {
            t->topic = strdup(name);
            if (t->topic) {
                t->refCount = nAUs;
                (void) top_schema(name, &ncols, &schema);
                d = (DataStackEntry *)malloc(ncols * sizeof(DataStackEntry));
                if (d) {
                    //printf("%p %d - created\n", t, t->refCount); fflush(stdout);
                    unpack(t->data, ncols, schema, d);
                    t->ncols = ncols;
                    t->theData = d;
                    return t;
                }
                free((void *)(t->topic));
            }
            free((void *)(t->data));
        }
        free((void *)t);
    }
    return NULL;
}

static pthread_mutex_t ev_mutex = PTHREAD_MUTEX_INITIALIZER;

Event *ev_reference(Event *event) {
    if (event) {
        pthread_mutex_lock(&ev_mutex);
        event->refCount++;
        pthread_mutex_unlock(&ev_mutex);
    }
    return event;
}

void ev_release(Event *event) {
    if (! event)
        return;
    pthread_mutex_lock(&ev_mutex);
    event->refCount--;
    //printf("%p %d\n", event, event->refCount); fflush(stdout);
    if (event->refCount == 0) {
        //printf("%p - freeing\n", event);
        free((void *)event->data);
        free((void *)event->topic);
        free((void *)event->theData);
        free((void *)event);
    }
    pthread_mutex_unlock(&ev_mutex);
}

char *ev_data(Event *event) {
    return event->data;
}

char *ev_topic(Event *event) {
    return event->topic;
}

int ev_theData(Event *event, DataStackEntry **dse) {
    *dse = event->theData;
    return event->ncols;
}

int ev_refCount(Event *event) {
    return event->refCount;
}

void ev_dump(Event *event) {
    int i;
    DataStackEntry *dse;

    dse = event->theData;
    fprintf(stderr, "Event for %s, refCount = %d, ncols = %d\n", event->topic,
            event->refCount, event->ncols);
    for (i = 0; i < event->ncols; i++) {
        dumpDataStackEntry(dse++, 1);
    }
}
