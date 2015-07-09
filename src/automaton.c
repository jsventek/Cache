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
 * Automaton ADT implementation
 */

#include "automaton.h"
#include "topic.h"
#include "adts/linkedlist.h"
#include "adts/tshashmap.h"
#include "adts/arraylist.h"
#include "machineContext.h"
#include "code.h"
#include "stack.h"
#include "timestamp.h"
#include "dsemem.h"
#include "topic.h"
#include "a_globals.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <setjmp.h>
#include <string.h>
#include <time.h>
#include "srpc/srpc.h"
#include "logdefs.h"

#define DEFAULT_HASH_TABLE_SIZE 20

struct automaton {
    short must_exit;
    short has_exited;
    unsigned long id;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    LinkedList *events;
    HashMap *topics;
    ArrayList *variables;
    ArrayList *index2vars;
    InstructionEntry *init;
    InstructionEntry *behav;
    RpcConnection rpc;
};

static TSHashMap *automatons = NULL;
static struct timespec delay = { 1, 0};
static pthread_mutex_t compile_lock = PTHREAD_MUTEX_INITIALIZER;

extern int a_parse(void);
extern void ap_init();
extern void a_init();

/*
 * keys used for storing jmp_buf and execution error information with thread
 */

pthread_key_t jmpbuf_key;
pthread_key_t execerr_key;

static void *thread_func(void *args) {
    Event *current;
    Automaton *self = (Automaton *)args;
    DataStackEntry *dse;
    MachineContext mc;
    char **keys;
    long i, n;
    jmp_buf begin;
    int execerr = 0;
    int ifbehav = 0;
    char buf[20];
    void *dummy;

    pthread_mutex_lock(&(self->lock));
    mc.variables = self->variables;
    mc.index2vars = self->index2vars;
    mc.stack = stack_create(0);
    mc.currentTopic = NULL;
    mc.au = self;
    reset(mc.stack);
    if (self->init) {
        if (! setjmp(begin)) {
            (void)pthread_setspecific(jmpbuf_key, (void *)begin);
            execute(&mc, self->init);
            execerr = 0;
            ifbehav = 1;
        } else
            execerr = 1;
    }
    pthread_mutex_unlock(&(self->lock));
    while (!execerr) {
        DataStackEntry d;
        long value;
        pthread_mutex_lock(&(self->lock));
        while (!self->must_exit && ll_size(self->events) == 0L) {
            pthread_cond_wait(&(self->cond), &(self->lock));
        }
        if (self->must_exit) {
            pthread_mutex_unlock(&(self->lock));
            break;
        }
        (void) ll_removeFirst(self->events, (void **)&current);
        if (! hm_get(self->topics, ev_topic(current), (void **)&value)) {
            pthread_mutex_unlock(&(self->lock));
            continue;
        }
        d.type = dEVENT;
        d.value.ev_v = current;
        (void)al_set(self->variables, dse_duplicate(d), value, (void **)&dse);
        if (dse) {
            if (dse->type == dEVENT)
                ev_release(dse->value.ev_v);
            dse_free(dse);
        }
        /* now execute program code */
        reset(mc.stack);
        mc.currentTopic = ev_topic(current);
        if (! setjmp(begin)) {
            (void)pthread_setspecific(jmpbuf_key, (void *)begin);
            execute(&mc, self->behav);
            execerr = 0;
        } else
            execerr = 1;
        pthread_mutex_unlock(&(self->lock));
    }

    /*
     * we arrive here for three possible reasons:
     * 1. self->must_exit was true, indicating that unregister was invoked
     * 2. execerr == 1 because of execution error in initialization code
     * 3. execerr == 1 because of execution error in behavior code
     *
     * still need to perform all of the cleanup; if execerr == 1, then must
     * send rpc to client application before disconnecting the rpc connection
     */

    debugf("Exited from repeat loop in automaton thread\n");
    pthread_mutex_lock(&(self->lock));
    self->has_exited++;

    /*
     * return storage associated with topic->variable mapping
     */

    keys = hm_keyArray(self->topics, &n);
    debugf("Unsubscribing from topics\n");
    for (i = 0L; i < n; i++) {
        void *datum;
        pthread_mutex_unlock(&(self->lock));/* if top_pub blocked on our lock */
        top_unsubscribe(keys[i], self->id); /* unsubscribe from the topic */
        pthread_mutex_lock(&(self->lock));
        hm_remove(self->topics, keys[i], &datum);
    }
    free(keys);
    hm_destroy(self->topics, NULL);	/* destroy the hash table */

    /*
     * now release all queued events
     */

    debugf("Releasing all queued events\n");
    while (ll_removeFirst(self->events, (void **)&current)) {
        ev_release(current);		/* decrement ref count */
    }
    ll_destroy(self->events, NULL);	/* delete the linked list */

    /*
     * now return storage associated with variable->value mapping
     */

    debugf("Returning storage associated with variables\n");
    debugf("Looping over all variables\n");
    for (i = al_size(self->variables) - 1; i >= 0; i--) {
        DataStackEntry *dx;
        char *s;
        debugf("Removing variable %ld from `variables' and 'index2vars'\n", i);
        (void) al_remove(self->variables, i, (void **)&dx);
        (void) al_remove(self->index2vars, i, (void **)&s);
        debugf("Removed variable %ld from `variables' and `index2vars'\n", i);
        if (dx->type == dEVENT)
            ev_release(dx->value.ev_v);
        else if (dx->type == dSEQUENCE || dx->type == dWINDOW)
            ;	/* assume seq and win inserted into map */
        else
            freeDSE(dx);
        dse_free(dx);			/* return the storage */
        free(s);
    }
    debugf("Destroying the arraylists\n");
    al_destroy(self->variables, NULL);	/* destroy the array list */
    al_destroy(self->index2vars, NULL); /* destroy the array list */

    /*
     * now return initialization and behavior code sequences and stack
     */

    debugf("Returning initialization and behavior code, and stack\n");
    free(self->init);
    free(self->behav);
    stack_destroy(mc.stack);

    /*
     * if execution error, send error message
     * disconnect rpc channel
     */

    if (execerr) {
        char *s = (char *)pthread_getspecific(execerr_key);
        Q_Decl(buf,1024);
        char resp[100];
        unsigned rlen, len;
        sprintf(buf, "100<|>%s execution error: %s<|>0<|>0<|>",
                (ifbehav) ? "behavior" : "initialization", s);
        free(s);
        len = strlen(buf) + 1;
        (void) rpc_call(self->rpc, Q_Arg(buf), len, resp, sizeof(resp), &rlen);
        self->has_exited++;
    }
    pthread_mutex_unlock(&(self->lock));
    rpc_disconnect(self->rpc);
    sprintf(buf, "%08lx", self->id);
    (void) tshm_remove(automatons, buf, &dummy);
    free(self);
    return NULL;
}

static void *timer_func(void *args) {
    struct timespec *delay = (struct timespec *)args;
    char *ts;

    while (1) {
        nanosleep(delay, NULL);
        ts = timestamp_to_string(timestamp_now());
        top_publish("Timer", ts);
        free(ts);
    }
    return NULL;
}

#define ID_CNTR_START 12345;
#define ID_CNTR_LIMIT 2000000000
static unsigned long a_id_cntr = ID_CNTR_START;

static unsigned long next_id(void) {
    unsigned long ans = a_id_cntr++;
    if (a_id_cntr > ID_CNTR_LIMIT)
        a_id_cntr = ID_CNTR_START;
    return ans;
}

void au_init(void) {
    pthread_t th;
    (void)pthread_key_create(&jmpbuf_key, NULL);
    (void)pthread_key_create(&execerr_key, NULL);
    automatons = tshm_create(25L, 10.0);
    (void) top_create("Timer", "1 tstamp/timestamp");
    (void) pthread_create(&th, NULL, timer_func, (void *)(&delay));
}

Automaton *au_create(char *program, RpcConnection rpc, char *ebuf) {
    Automaton *au;
    void *dummy;
    pthread_t pthr;

    au = (Automaton *)malloc(sizeof(Automaton));
    if (au) {
        au->id = next_id();
        au->must_exit = 0;
        au->has_exited = 0;
        pthread_mutex_init(&(au->lock), NULL);
        pthread_cond_init(&(au->cond), NULL);
        au->rpc = rpc;
        au->events = ll_create();
        if (au->events) {
            char buf[20];
            jmp_buf begin;
            pthread_mutex_lock(&compile_lock);
            if (!setjmp(begin)) {	/* not here because of a longjmp */
                (void)pthread_setspecific(jmpbuf_key, (void *)begin);
                ap_init(program);
                a_init();
                initcode();
                if (! a_parse()) {	/* successful compilation */
                    size_t N;
                    Iterator *it;
                    N = initSize * sizeof(InstructionEntry);
                    if (N) {
                        au->init = (InstructionEntry *)malloc(N);
                        memcpy(au->init, initialization, N);
                    } else
                        au->init = NULL;
                    N = behavSize * sizeof(InstructionEntry);
                    au->behav = (InstructionEntry *)malloc(N);
                    memcpy(au->behav, behavior, N);
                    au->variables = variables;
		    au->index2vars = index2vars;
                    au->topics = topics;
                    sprintf(buf, "%08lx", au->id);
                    (void) tshm_put(automatons, buf, au, &dummy);
                    pthread_mutex_lock(&(au->lock));
                    it = hm_it_create(topics);
                    if (it) {
                        while (it_hasNext(it)) {
                            HMEntry *hme;
                            (void) it_next(it, (void **)&hme);
                            top_subscribe(hmentry_key(hme), au->id);
                        }
                        it_destroy(it);
                    }
                    variables = NULL;
                    topics = NULL;
                    pthread_mutex_unlock(&(au->lock));
                    pthread_create(&pthr, NULL, thread_func, (void *)au);
                    pthread_mutex_unlock(&compile_lock);
                    return au;
                }
            } else {
                char *s = (char *)pthread_getspecific(execerr_key);
                strcpy(ebuf, s);
                free(s);
            } /* need else here to set up appropriate status return */
            pthread_mutex_unlock(&compile_lock);
        }
        ll_destroy(au->events, NULL);
    }
    free((void *)au);
    return NULL;
}

int au_destroy(unsigned long id) {
    char buf[20];
    Automaton *au;
    int ans = 0;
    sprintf(buf, "%08lx", id);
    if (tshm_get(automatons, buf, (void **)&au)) {
        void *dummy;
        int must_free = 0;
        (void) tshm_remove(automatons, buf, &dummy);
        pthread_mutex_lock(&(au->lock));
        if (! au->has_exited) {
            au->must_exit = 1;
            pthread_cond_signal(&(au->cond));
            ans = 1;
        } else {
            must_free = 1;
        }
        pthread_mutex_unlock(&(au->lock));
        if (must_free)
            free(au);
    }
    return ans;
}

void au_publish(unsigned long id, Event *event) {
    Automaton *au = au_au(id);
    if (! au)
        return;	/* probably need to ev_release event here */
    pthread_mutex_lock(&(au->lock));
    if (! au->must_exit && ! au->has_exited) {
        (void)ll_addFirst(au->events, event);
        pthread_cond_signal(&(au->cond));
    }
    pthread_mutex_unlock(&(au->lock));
}

unsigned long au_id(Automaton *au) {
    return au->id;
}

Automaton *au_au(unsigned long id) {
    char buf[20];
    Automaton *au;
    sprintf(buf, "%08lx", id);
    if (! tshm_get(automatons, buf, (void **)&au))
        au = NULL;
    return au;
}

RpcConnection au_rpc(Automaton *au) {
    return au->rpc;
}
