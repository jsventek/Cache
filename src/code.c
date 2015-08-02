/*
 * Copyright (c) 2012, Court of the University of Glasgow
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
 * execution and stack manipulation routines
 */

#include "dataStackEntry.h"
#include "adts/hashmap.h"
#include "code.h"
#include "event.h"
#include "topic.h"
#include "timestamp.h"
#include "a_globals.h"
#include "dsemem.h"
#include "ptable.h"
#include "typetable.h"
#include "sqlstmts.h"
#include "hwdb.h"
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>


#define NSTACK 2560
#define NPROG 20000

static InstructionEntry initbuf[NPROG];
static InstructionEntry behavbuf[NPROG];

int iflog = 0;
int initSize, behavSize;
InstructionEntry *progp, *startp;
InstructionEntry *initialization = initbuf;
InstructionEntry *behavior = behavbuf;

static char *varName(MachineContext *mc, long index) {
    char *s;

    (void) al_get(mc->index2vars, index, (void **)&s);
    return s;
}

void dumpDataStackEntry (DataStackEntry *d, int verbose) {
    if (verbose)
        fprintf(stderr, "type = %d, value = ", d->type);
    switch(d->type) {
        char *ts;
    case dBOOLEAN:
        if (d->value.bool_v)
            fprintf(stderr, "T");
        else
            fprintf(stderr, "F");
        break;
    case dINTEGER:
        fprintf(stderr, "%lld", d->value.int_v);
        break;
    case dDOUBLE:
        fprintf(stderr, "%.8f", d->value.dbl_v);
        break;
    case dTSTAMP:
        ts = timestamp_to_string(d->value.tstamp_v);
        fprintf(stderr, "%s", ts);
        free(ts);
        break;
    case dSTRING:
    case dIDENT:
    case dTIDENT:
        fprintf(stderr, "%s", d->value.str_v);
        break;
    case dEVENT:
        fprintf(stderr, "A_Tuple");
        break;
    case dMAP:
        fprintf(stderr, "A_Map");
        break;
    case dWINDOW:
        fprintf(stderr, "A_Window");
        break;
    case dITERATOR:
        fprintf(stderr, "An_Iterator");
        break;
    case dSEQUENCE:
        fprintf(stderr, "A_Sequence");
        break;
    case dPTABLE:
        fprintf(stderr, "A_PersistentTable");
        break;
    }
    if (verbose)
        fprintf(stderr, "\n");
}

DataStackEntry *dse_duplicate(DataStackEntry d) {
    DataStackEntry *p = dse_alloc();
    if (p)
        memcpy(p, &d, sizeof(DataStackEntry));
    return p;
}

GAPLMap *map_duplicate(GAPLMap m) {
    GAPLMap *p = (GAPLMap *)malloc(sizeof(GAPLMap));
    if (p)
        memcpy(p, &m, sizeof(GAPLMap));
    return p;
}

GAPLWindow *win_duplicate(GAPLWindow w) {
    GAPLWindow *p = (GAPLWindow *)malloc(sizeof(GAPLWindow));
    if (p)
        memcpy(p, &w, sizeof(GAPLWindow));
    return p;
}

GAPLWindowEntry *we_duplicate(GAPLWindowEntry w) {
    GAPLWindowEntry *p = (GAPLWindowEntry *)malloc(sizeof(GAPLWindowEntry));
    if (p)
        memcpy(p, &w, sizeof(GAPLWindowEntry));
    return p;
}

GAPLIterator *iter_duplicate(GAPLIterator it) {
    GAPLIterator *p = (GAPLIterator *)malloc(sizeof(GAPLIterator));
    if (p)
        memcpy(p, &it, sizeof(GAPLIterator));
    return p;
}

GAPLSequence *seq_duplicate(GAPLSequence s) {
    GAPLSequence *p = (GAPLSequence *)malloc(sizeof(GAPLSequence));
    if (p)
        memcpy(p, &s, sizeof(GAPLSequence));
    return p;
}

void initcode(void) {
    progp = initialization;
    startp = progp;
    initSize = 0;
}

void switchcode(void) {
    initSize = (progp - startp);
    progp = behavior;
    startp = progp;
    behavSize = 0;
}

void endcode(void) {
    behavSize = (progp - startp);
}

InstructionEntry *code(int ifInst, Inst f, DataStackEntry *d, char *what,
                       int lineno) {
    InstructionEntry *savedprogp = progp;
    if (iflog) fprintf(stderr, "ifInst = %d, f = %p, d = %p, n = %d, \"%s\"\n",
                           ifInst, f, d, lineno, what);
    if (progp >= startp+NPROG)
        comperror("Program too large", NULL);
    else {
        if (ifInst) {
            progp->type = FUNC;
            progp->u.op = f;
        } else {
            progp->type = DATA;
            progp->u.immediate = *d;
        }
        progp->label = what;
        progp->lineno = lineno;
        progp++;
    }
    return savedprogp;
}

void execute(MachineContext *mc, InstructionEntry *p) {
    mc->pc = p;
    while (mc->pc->u.op != STOP) {
        (*((mc->pc++)->u.op))(mc);
    }
}

static void printIE(InstructionEntry *p, FILE *fd) {
    switch(p->type) {
    case FUNC:
        fprintf(fd, "Function pointer (%s) %p\n", p->label, p->u.op);
        break;
    case DATA:
        fprintf(fd, "Immediate data (%s), ", p->label);
        dumpDataStackEntry(&(p->u.immediate), 1);
        break;
    case PNTR:
        fprintf(fd, "Offset from PC (%s), %d\n", p->label, p->u.offset);
        break;
    }
}

static void dumpProgram(InstructionEntry *p) {
    if (! iflog)
        return;
    while (!(p->type == FUNC && p->u.op == STOP)) {
        printIE(p, stderr);
        p++;
    }
}

void dumpProgramBlock(InstructionEntry *p, int n) {
    int savediflog = iflog;
    iflog = 1;
    while (n > 0) {
        printIE(p, stderr);
        p++;
        n--;
    }
    iflog = savediflog;
}

void dumpCompilationResults(unsigned long id, ArrayList *v, ArrayList *i2v,
                            InstructionEntry *init, int initSize,
                            InstructionEntry *behav, int behavSize) {
    long i, n;

    fprintf(stderr, "=== Compilation results for automaton %08lx\n", id);
    fprintf(stderr, "====== variables\n");
    n = al_size(v);
    for (i = 0; i < n; i++) {
        DataStackEntry *d;
        char *s;
        (void) al_get(v, i, (void **)&d);
        (void) al_get(i2v, i, (void **)&s);
        fprintf(stderr, "%s: ", s);
        switch(d->type) {
        case dBOOLEAN:
            fprintf(stderr, "bool");
            break;
        case dINTEGER:
            fprintf(stderr, "int");
            break;
        case dDOUBLE:
            fprintf(stderr, "real");
            break;
        case dTSTAMP:
            fprintf(stderr, "tstamp");
            break;
        case dSTRING:
            fprintf(stderr, "string");
            break;
        case dEVENT:
            fprintf(stderr, "tuple");
            break;
        case dMAP:
            fprintf(stderr, "map");
            break;
        case dIDENT:
            fprintf(stderr, "identifier");
            break;
        case dWINDOW:
            fprintf(stderr, "window");
            break;
        case dITERATOR:
            fprintf(stderr, "iterator");
            break;
        case dSEQUENCE:
            fprintf(stderr, "sequence");
            break;
        case dPTABLE:
            fprintf(stderr, "PTable");
            break;
        }
        fprintf(stderr, "\n");
    }
    fprintf(stderr, "====== initialization instructions\n");
    dumpProgramBlock(init, initSize);
    fprintf(stderr, "====== behavior instructions\n");
    dumpProgramBlock(behav, behavSize);
    fprintf(stderr, "=== End of comp results for automaton %08lx\n", id);
}

static void logit(char *s, Automaton *au) {
    fprintf(stderr, "%08lx: %s", au_id(au), s);
}

void constpush(MachineContext *mc) {
    if (iflog) {
        logit("constpush called, ", mc->au);
        dumpDataStackEntry(&(mc->pc->u.immediate), 1);
    }
    push(mc->stack, mc->pc->u.immediate);
    mc->pc++;
}

void varpush(MachineContext *mc) {
    if (iflog) {
        logit("varpush called, ", mc->au);
        dumpDataStackEntry(&(mc->pc->u.immediate), 1);
    }
    push(mc->stack, mc->pc->u.immediate);
    mc->pc++;
}

void add(MachineContext *mc) {
    DataStackEntry d1, d2;
    if (iflog) logit("add called\n", mc->au);
    d2 = pop(mc->stack);
    d1 = pop(mc->stack);
    if ((d1.type == d2.type)) {
        switch(d1.type) {
        case dINTEGER:
            d1.value.int_v += d2.value.int_v;
            break;
        case dDOUBLE:
            d1.value.dbl_v += d2.value.dbl_v;
            break;
        default:
            execerror(mc->pc->lineno, "add of non-numeric types", NULL);
        }
    } else
        execerror(mc->pc->lineno, "add of two different types", NULL);
    d1.flags = 0;
    push(mc->stack, d1);
}

void subtract(MachineContext *mc) {
    DataStackEntry d1, d2;
    if (iflog) logit("subtract called\n", mc->au);
    d2 = pop(mc->stack);
    d1 = pop(mc->stack);
    if ((d1.type == d2.type)) {
        switch(d1.type) {
        case dINTEGER:
            d1.value.int_v -= d2.value.int_v;
            break;
        case dDOUBLE:
            d1.value.dbl_v -= d2.value.dbl_v;
            break;
        default:
            execerror(mc->pc->lineno, "subtract of non-numeric types", NULL);
        }
    } else
        execerror(mc->pc->lineno, "subtract of two different types", NULL);
    d1.flags = 0;
    push(mc->stack, d1);
}

void multiply(MachineContext *mc) {
    DataStackEntry d1, d2;
    if (iflog) logit("multiply called\n", mc->au);
    d2 = pop(mc->stack);
    d1 = pop(mc->stack);
    if ((d1.type == d2.type)) {
        switch(d1.type) {
        case dINTEGER:
            d1.value.int_v *= d2.value.int_v;
            break;
        case dDOUBLE:
            d1.value.dbl_v *= d2.value.dbl_v;
            break;
        default:
            execerror(mc->pc->lineno, "multiply of non-numeric types", NULL);
        }
    } else
        execerror(mc->pc->lineno, "multiply of two different types", NULL);
    d1.flags = 0;
    push(mc->stack, d1);
}

void divide(MachineContext *mc) {
    DataStackEntry d1, d2;
    if (iflog) logit("divide called\n", mc->au);
    d2 = pop(mc->stack);
    d1 = pop(mc->stack);
    if ((d1.type == d2.type)) {
        switch(d1.type) {
        case dINTEGER:
            d1.value.int_v /= d2.value.int_v;
            break;
        case dDOUBLE:
            d1.value.dbl_v /= d2.value.dbl_v;
            break;
        default:
            execerror(mc->pc->lineno, "divide of non-numeric types", NULL);
        }
    } else
        execerror(mc->pc->lineno, "divide of two different types", NULL);
    d1.flags = 0;
    push(mc->stack, d1);
}

void modulo(MachineContext *mc) {
    DataStackEntry d1, d2;
    if (iflog) logit("modulo called\n", mc->au);
    d2 = pop(mc->stack);
    d1 = pop(mc->stack);
    if ((d1.type == d2.type)) {
        switch(d1.type) {
        case dINTEGER:
            d1.value.int_v %= d2.value.int_v;
            break;
        default:
            execerror(mc->pc->lineno, "modulo of non-integer types", NULL);
        }
    } else
        execerror(mc->pc->lineno, "modulo of two different types", NULL);
    d1.flags = 0;
    push(mc->stack, d1);
}

void bitOr(MachineContext *mc) {
    DataStackEntry d1, d2;
    if (iflog) logit("bitOr called\n", mc->au);
    d2 = pop(mc->stack);
    d1 = pop(mc->stack);
    if ((d1.type == d2.type)) {
        switch(d1.type) {
        case dINTEGER:
            d1.value.int_v |= d2.value.int_v;
            break;
        default:
            execerror(mc->pc->lineno, "bitOr of non-integer types", NULL);
        }
    } else
        execerror(mc->pc->lineno, "bitOr of two different types", NULL);
    d1.flags = 0;
    push(mc->stack, d1);
}

void bitAnd(MachineContext *mc) {
    DataStackEntry d1, d2;
    if (iflog) logit("bitAnd called\n", mc->au);
    d2 = pop(mc->stack);
    d1 = pop(mc->stack);
    if ((d1.type == d2.type)) {
        switch(d1.type) {
        case dINTEGER:
            d1.value.int_v &= d2.value.int_v;
            break;
        default:
            execerror(mc->pc->lineno, "bitAnd of non-integer types", NULL);
        }
    } else
        execerror(mc->pc->lineno, "bitAnd of two different types", NULL);
    d1.flags = 0;
    push(mc->stack, d1);
}

void negate(MachineContext *mc) {
    DataStackEntry d;
    if (iflog) logit("negate called\n", mc->au);
    d = pop(mc->stack);
    switch(d.type) {
    case dINTEGER:
        d.value.int_v = -d.value.int_v;
        break;
    case dDOUBLE:
        d.value.dbl_v = -d.value.dbl_v;
        break;
    default:
        execerror(mc->pc->lineno, "negate of non-numeric type", NULL);
    }
    d.flags = 0;
    push(mc->stack, d);
}

static void printvalue(DataStackEntry d, FILE *fd) {
    char b[100];
    switch(d.type) {
    case dINTEGER:
        fprintf(fd, "%lld", d.value.int_v);
        break;
    case dDOUBLE:
        sprintf(b, "%.10f", d.value.dbl_v);
        fprintf(fd, "%s", b);
        if (strchr(b, '.') == NULL)
            fprintf(fd, ".0");
        break;
    case dSTRING:
    case dIDENT:
        fprintf(fd, "%s", d.value.str_v);
        break;
    case dTSTAMP: {
        char *p;
        p = timestamp_to_string(d.value.tstamp_v);
        fprintf(fd, "%s", p);
        free(p);
        break;
    }
    case dBOOLEAN: {
        fprintf(fd, "%s", (d.value.bool_v) ? "true" : "false");
        break;
    }
    case dSEQUENCE: {
        int i;
        DataStackEntry *dse = d.value.seq_v->entries;
        fprintf(fd, "{");
        for (i = 0; i < d.value.seq_v->used; i++) {
            fprintf(fd, "%s", (i == 0) ? " " : ", ");
            printvalue(*dse++, stdout);
        }
        fprintf(fd, " }");
        break;
    }
    default:
        fprintf(fd, "unknown type: %d", d.type);
    }
}

void printDSE(DataStackEntry *d, FILE *fd) {
    printvalue(*d, fd);
}

static int packvalue(DataStackEntry d, char *s) {
    int n;
    switch(d.type) {
    case dINTEGER:
        n = sprintf(s, "%lld", d.value.int_v);
        break;
    case dDOUBLE:
        n = sprintf(s, "%.2f", d.value.dbl_v);
        break;
    case dSTRING:
        n = sprintf(s, "%s", d.value.str_v);
        break;
    case dTSTAMP: {
        char *p;
        p = timestamp_to_string(d.value.tstamp_v);
        n = sprintf(s, "%s", p);
        free(p);
        break;
    }
    case dBOOLEAN:
        n = sprintf(s, "%s", (d.value.bool_v) ? "true" : "false");
        break;
    }
    return n;
}

/* free any heap storage associated with temporary data stack entry */
void freeDSE(DataStackEntry *d) {
    switch(d->type) {
    case dSTRING:
    case dIDENT: {
        if (d->value.str_v && (d->flags & MUST_FREE))
            free(d->value.str_v);
        break;
    }
    case dITERATOR: {
        if (d->value.iter_v) {
            GAPLIterator *it = d->value.iter_v;
            if (it->type == dMAP && it->u.m_idents)
                free(it->u.m_idents);
            else if (it->type == dPTABLE && it->u.m_idents) {
                int i;
                for (i = 0; i < it->size; i++)
                    free(it->u.m_idents[i]);
                free(it->u.m_idents);
            } else if (it->u.w_it)
                it_destroy(it->u.w_it);
            free(d->value.iter_v);
        }
        break;
    }
    case dSEQUENCE: {
        /*printf("[alex] in freeDSE, delete ");
        printDSE(d, stdout);
        printf("\n");*/
        if (d->value.seq_v) {
            GAPLSequence *seq = d->value.seq_v;
            if (seq->entries) {
                int i;
                DataStackEntry *dse = seq->entries;
                for (i = 0; i < seq->used; i++)
                    freeDSE(dse++);
                free(seq->entries);
            }
            free(seq);
        }
        break;
    }
    case dMAP: {
        if (d->value.map_v) {
            if (d->value.map_v->hm) {
                char **keys;
                long i, n;
                void *datum;
                keys = hm_keyArray(d->value.map_v->hm, &n);
                for (i = 0; i < n; i++) {
                    DataStackEntry *dx;
                    (void)hm_remove(d->value.map_v->hm, keys[i], &datum);
                    dx = (DataStackEntry *)datum;
                    freeDSE(dx);
                    dse_free(dx);
                }
                free(keys);
                hm_destroy(d->value.map_v->hm, NULL);
            }
            free(d->value.map_v);
        }
        break;
    }
    case dWINDOW: {
        if (d->value.win_v) {
            if (d->value.win_v->ll) {
                GAPLWindow *w = d->value.win_v;
                GAPLWindowEntry *we;

                while (ll_removeFirst(w->ll, (void **)&we)) {
                    freeDSE(&(we->dse));
                    free(we);
                }
                ll_destroy(w->ll, NULL);
            }
            free(d->value.win_v);
        }
        break;
    }
    }
}

void print(MachineContext *mc) {
    DataStackEntry d = pop(mc->stack);
    if (iflog) logit("print called\n", mc->au);
    printf("\t");
    switch(d.type) {
    case dINTEGER:
    case dDOUBLE:
    case dSTRING:
    case dIDENT:
    case dTSTAMP:
    case dBOOLEAN:
        printvalue(d, stdout);
        break;
    case dWINDOW: {
        GAPLWindow *w = d.value.win_v;
        Iterator *it = ll_it_create(w->ll);
        GAPLWindowEntry *we;
        int n = 0;

        while (it_hasNext(it)) {
            (void) it_next(it, (void **)&we);
            if (n++ > 0)
                printf(" ");
            switch(we->dse.type) {
            case dINTEGER:
            case dDOUBLE:
                printvalue(we->dse, stdout);
                break;
            default:
                printf("odd window type");
                break;
            }
        }
        it_destroy(it);
        break;
    }
    case dSEQUENCE: {
        GAPLSequence *s = d.value.seq_v;
        int i;
        for (i = 0; i < s->used; i++) {
            if (i > 0)
                printf("<|>");
            printvalue(s->entries[i], stdout);
        }
        break;
    }
    default:
        printf("unknown type: %d\n", d.type);
    }
    printf("\n");
    fflush(stdout);
    if (!(d.flags & NOTASSIGN))
        freeDSE(&d);
}

void destroy(MachineContext *mc) {
    DataStackEntry *v;
    DataStackEntry name = mc->pc->u.immediate;
    void *dummy;
    int t;
    long index;

    if (iflog) logit("destroy called\n", mc->au);
    index = (long)name.value.int_v;
    (void)al_get(mc->variables, index, (void **)&v);
    /* void destroy(ident|iter|map|seq|win) */
    t = v->type;
    if (t != dIDENT && t != dITERATOR && t != dMAP &&
            t != dSEQUENCE && t != dWINDOW && t != dPTABLE)
        execerror(mc->pc->lineno,
                  "attempt to destroy an instance of a basic type",
                  varName(mc, index));
    freeDSE(v);
    v->flags = 0;
    switch(v->type) {
    case dIDENT:
        v->value.str_v = NULL;
        break;
    case dITERATOR:
        v->value.iter_v = NULL;
        break;
    case dMAP:
        v->value.map_v = NULL;
        break;
    case dSEQUENCE:
        v->value.seq_v = NULL;
        break;
    case dWINDOW:
        v->value.win_v = NULL;
        break;
    case dPTABLE:
        v->value.str_v = NULL;
        break;
    }
    (void)al_set(mc->variables, v, index, &dummy);
    mc->pc = mc->pc + 1;
}

void eval(MachineContext *mc) {
    DataStackEntry *v;
    DataStackEntry d = pop(mc->stack);
    long index;

    if (iflog) logit("eval called\n", mc->au);
    index = (long)d.value.int_v;
    (void) al_get(mc->variables, index, (void **)&v);
    push(mc->stack, *v);
}

void extract(MachineContext *mc) {
    DataStackEntry *v, ndx, vbl;
    Event *t;
    long index;

    if (iflog) logit("extract called\n", mc->au);
    ndx = pop(mc->stack);
    vbl = pop(mc->stack);
    index = (long)vbl.value.int_v;
    (void) al_get(mc->variables, index, (void **)&v);
    if (v->type != dEVENT)
        execerror(mc->pc->lineno, "variable not an event: ", varName(mc, index));
    t = (Event *)(v->value.ev_v);
    if (! t)
        execerror(mc->pc->lineno, "Tuple is null: ", varName(mc, index));
    (void) ev_theData(t, &v);
    push(mc->stack, v[ndx.value.int_v]);
}

static int simple_type(DataStackEntry *d) {
    int t = d->type;

    return (t == dBOOLEAN || t == dINTEGER || t == dDOUBLE || t == dTSTAMP);
}

void assign(MachineContext *mc) {
    DataStackEntry d1, d2, *d;
    void *v;
    int t;
    long index;

    if (iflog) logit("assign called\n", mc->au);

    d1 = pop(mc->stack);
    d2 = pop(mc->stack);
    if (!simple_type(&d2) && (d2.flags & NOTASSIGN))
        execerror(mc->pc->lineno, "attempt to create an alias: ", d1.value.str_v);
    t = d2.type;
    index = (long)d1.value.int_v;
    (void) al_get(mc->variables, index, (void **)&d);
    if (t != d->type)
        execerror(mc->pc->lineno,
                  "lhs and rhs of assignment of different types",
                  varName(mc, index));
    if (d2.flags & DUPLICATE) {
        d2.value.str_v = strdup(d2.value.str_v);
        d2.flags &= ~DUPLICATE;		/* make sure duplicate flag false */
        d2.flags |= MUST_FREE;
    }
    d2.flags |= NOTASSIGN;		/* prevent aliasing */
    (void)al_set(mc->variables, dse_duplicate(d2), index, &v);
    if (v && t != dSEQUENCE && t != dWINDOW) {
        freeDSE((DataStackEntry *)v);
    }
    dse_free((DataStackEntry *)v);
}

void pluseq(MachineContext *mc) {
    DataStackEntry d1, d2, *d;
    void *v;
    long index;

    if (iflog) logit("pluseq called\n", mc->au);

    d1 = pop(mc->stack);
    d2 = pop(mc->stack);
    index = (long)d1.value.int_v;
    (void)al_get(mc->variables, index, (void **)&d);
    if (d->type != dINTEGER)
        execerror(mc->pc->lineno, "pluseq of non-integer", varName(mc, index));
    d->value.int_v += d2.value.int_v;
    (void) al_set(mc->variables, d, index, &v);
}

void minuseq(MachineContext *mc) {
    DataStackEntry d1, d2, *d;
    void *v;
    long index;

    if (iflog) logit("minuseq called\n", mc->au);

    d1 = pop(mc->stack);
    d2 = pop(mc->stack);
    index = (long)d1.value.int_v;
    (void)al_get(mc->variables, index, (void **)&d);
    if (d->type != dINTEGER)
        execerror(mc->pc->lineno, "minuseq of non-integer", varName(mc, index));
    d->value.int_v -= d2.value.int_v;
    (void) al_set(mc->variables, d, index, &v);
}

static int compare(int lineno, DataStackEntry *d1, DataStackEntry *d2) {
    int t1 = d1->type;
    int t2 = d2->type;
    int a = -1;                        /* assume d1 < d2 */

    if (t1 == t2) {
        switch(t1) {
        case dBOOLEAN:
            if (d1->value.bool_v == d2->value.bool_v)
                a = 0;
            else if (d1->value.bool_v > d2->value.bool_v)
                a = 1;
            break;
        case dINTEGER:
            if (d1->value.int_v == d2->value.int_v)
                a = 0;
            else if (d1->value.int_v > d2->value.int_v)
                a = 1;
            break;
        case dDOUBLE:
            if (d1->value.dbl_v == d2->value.dbl_v)
                a = 0;
            else if (d1->value.dbl_v > d2->value.dbl_v)
                a = 1;
            break;
        case dTSTAMP:
            if (d1->value.tstamp_v == d2->value.tstamp_v)
                a = 0;
            else if (d1->value.tstamp_v > d2->value.tstamp_v)
                a = 1;
            break;
        case dSTRING:
            a = strcmp(d1->value.str_v, d2->value.str_v);
            break;
        default:
            execerror(lineno, "attempting to compare structured datatypes", NULL);
            break;
        }
    } else
        execerror(lineno, "attempting to compare different data types", NULL);
    return a;
}

void gt(MachineContext *mc) {
    DataStackEntry d1, d2;
    int a;
    if (iflog) logit("gt called\n", mc->au);
    d2 = pop(mc->stack);
    d1 = pop(mc->stack);
    a = compare(mc->pc->lineno, &d1, &d2);
    if (a > 0)
        d1.value.bool_v = 1;
    else
        d1.value.bool_v = 0;
    d1.type = dBOOLEAN;
    d1.flags = 0;
    push(mc->stack, d1);
}

void ge(MachineContext *mc) {
    DataStackEntry d1, d2;
    int a;
    if (iflog) logit("ge called\n", mc->au);
    d2 = pop(mc->stack);
    d1 = pop(mc->stack);
    a = compare(mc->pc->lineno, &d1, &d2);
    if (a >= 0)
        d1.value.bool_v = 1;
    else
        d1.value.bool_v = 0;
    d1.type = dBOOLEAN;
    d1.flags = 0;
    push(mc->stack, d1);
}

void lt(MachineContext *mc) {
    DataStackEntry d1, d2;
    int a;
    if (iflog) logit("lt called\n", mc->au);
    d2 = pop(mc->stack);
    d1 = pop(mc->stack);
    a = compare(mc->pc->lineno, &d1, &d2);
    if (a < 0)
        d1.value.bool_v = 1;
    else
        d1.value.bool_v = 0;
    d1.type = dBOOLEAN;
    d1.flags = 0;
    push(mc->stack, d1);
}

void le(MachineContext *mc) {
    DataStackEntry d1, d2;
    int a;
    if (iflog) logit("le called\n", mc->au);
    d2 = pop(mc->stack);
    d1 = pop(mc->stack);
    a = compare(mc->pc->lineno, &d1, &d2);
    if (a <= 0)
        d1.value.bool_v = 1;
    else
        d1.value.bool_v = 0;
    d1.type = dBOOLEAN;
    d1.flags = 0;
    push(mc->stack, d1);
}

void eq(MachineContext *mc) {
    DataStackEntry d1, d2;
    if (iflog) logit("eq called\n", mc->au);
    int a;
    d2 = pop(mc->stack);
    d1 = pop(mc->stack);
    a = compare(mc->pc->lineno, &d1, &d2);
    if (a == 0)
        d1.value.bool_v = 1;
    else
        d1.value.bool_v = 0;
    d1.type = dBOOLEAN;
    d1.flags = 0;
    push(mc->stack, d1);
}

void ne(MachineContext *mc) {
    DataStackEntry d1, d2;
    int a;
    if (iflog) logit("ne called\n", mc->au);
    d2 = pop(mc->stack);
    d1 = pop(mc->stack);
    a = compare(mc->pc->lineno, &d1, &d2);
    if (a != 0)
        d1.value.bool_v = 1;
    else
        d1.value.bool_v = 0;
    d1.type = dBOOLEAN;
    d1.flags = 0;
    push(mc->stack, d1);
}

void and(MachineContext *mc) {
    DataStackEntry d1, d2;
    if (iflog) logit("and called\n", mc->au);
    d2 = pop(mc->stack);
    d1 = pop(mc->stack);
    if (d1.type == dBOOLEAN && d2.type == dBOOLEAN)
        if (d1.value.bool_v && d2.value.bool_v)
            d1.value.bool_v = 1;
        else
            d1.value.bool_v = 0;
    else
        execerror(mc->pc->lineno, "AND of non boolean expressions", NULL);
    d1.flags = 0;
    push(mc->stack, d1);
}

void or(MachineContext *mc) {
    DataStackEntry d1, d2;
    if (iflog) logit("or called\n", mc->au);
    d2 = pop(mc->stack);
    d1 = pop(mc->stack);
    if (d1.type == dBOOLEAN && d2.type == dBOOLEAN)
        if (d1.value.bool_v || d2.value.bool_v)
            d1.value.bool_v = 1;
        else
            d1.value.bool_v = 0;
    else
        execerror(mc->pc->lineno, "OR of non boolean expressions", NULL);
    d1.flags = 0;
    push(mc->stack, d1);
}

void not(MachineContext *mc) {
    DataStackEntry d;
    if (iflog) logit("not called\n", mc->au);
    d = pop(mc->stack);
    if (d.type == dBOOLEAN)
        d.value.bool_v = 1 - d.value.bool_v;
    else
        execerror(mc->pc->lineno, "NOT of non-boolean expression", NULL);
    d.flags = 0;
    push(mc->stack, d);
}

void whilecode(MachineContext *mc) {
    DataStackEntry d;
    InstructionEntry *base = mc->pc - 1;	/* autoincrement happened before call */
    InstructionEntry *body = base + mc->pc->u.offset;
    InstructionEntry *nextStmt = base + (mc->pc+1)->u.offset;
    InstructionEntry *condition = mc->pc + 2;
    if (iflog) logit("whilecode called\n", mc->au);

    if (iflog) logit("=====> Dumping condition\n", mc->au);
    dumpProgram(condition);
    if (iflog) logit("=====> Dumping body\n", mc->au);
    dumpProgram(body);
    execute(mc, condition);
    d = pop(mc->stack);
    if (iflog)
        fprintf(stderr, "%08lx: condition is %d\n", au_id(mc->au), d.value.bool_v);
    while (d.value.bool_v) {
        execute(mc, body);
        execute(mc, condition);
        d = pop(mc->stack);
        if (iflog)
            fprintf(stderr, "%08lx: condition is %d\n", au_id(mc->au), d.value.bool_v);
    }
    mc->pc = nextStmt;
}

void ifcode(MachineContext *mc) {
    DataStackEntry d;
    InstructionEntry *base = mc->pc - 1;
    InstructionEntry *thenpart = base + mc->pc->u.offset;
    InstructionEntry *elsepart = base + (mc->pc+1)->u.offset;
    InstructionEntry *nextStmt = base + (mc->pc+2)->u.offset;
    InstructionEntry *condition = mc->pc + 3;

    if (iflog) {
        logit("ifcode called\n", mc->au);
        fprintf(stderr, "   PC        address %p\n", mc->pc);
        fprintf(stderr, "   condition address %p\n", condition);
        fprintf(stderr, "   thenpart  address %p\n", thenpart);
        fprintf(stderr, "   elsepart  address %p\n", elsepart);
        fprintf(stderr, "   nextstmt  address %p\n", nextStmt);
        logit("=====> Dumping condition\n", mc->au);
        dumpProgram(condition);
        logit("=====> Dumping thenpart\n", mc->au);
        dumpProgram(thenpart);
        if(elsepart != base) {
            logit("=====> Dumping elsepart\n", mc->au);
            dumpProgram(elsepart);
        } else
            logit("=====> No elsepart to if\n", mc->au);
    }
    execute(mc, condition);
    d = pop(mc->stack);
    if (iflog)
        fprintf(stderr, "%08lx: condition is %d\n", au_id(mc->au), d.value.bool_v);
    if (d.value.bool_v)
        execute(mc, thenpart);
    else if (elsepart != base)
        execute(mc, elsepart);
    mc->pc = nextStmt;
}

void newmap(MachineContext *mc) {
    GAPLMap m;
    DataStackEntry d;
    DataStackEntry type = mc->pc->u.immediate;
    if (iflog) logit("newmap called\n", mc->au);

    m.type = type.value.int_v;
    m.hm = hm_create(0L, 10.0);
    d.type = dMAP;
    d.flags = 0;
    d.value.map_v = map_duplicate(m);
    push(mc->stack, d);
    mc->pc = mc->pc + 1;
}

void newwindow(MachineContext *mc) {
    GAPLWindow w;
    DataStackEntry d;
    DataStackEntry dtype = mc->pc->u.immediate;
    DataStackEntry ctype = (mc->pc+1)->u.immediate;
    DataStackEntry csize = (mc->pc+2)->u.immediate;
    if (iflog) logit("newwindow called\n", mc->au);

    w.dtype = dtype.value.int_v;
    w.wtype = ctype.value.int_v;
    w.rows_secs = csize.value.int_v;
    w.ll = ll_create();
    d.type = dWINDOW;
    d.flags = 0;
    d.value.win_v = win_duplicate(w);
    push(mc->stack, d);
    mc->pc = mc->pc + 3;
}

static char *concat(int type, long long nargs, DataStackEntry *args) {
    char buf[1024], *p;
    char b[100];
    int i;

    p = buf;
    for (i = 0; i < nargs; i++) {
        char *q;
        if (type == dIDENT && i > 0)
            p = p + sprintf(p, "|");
        switch(args[i].type) {
        case dBOOLEAN:
            p = p + sprintf(p, "%s", (args[i].value.bool_v)?"T":"F");
            break;
        case dINTEGER:
            p = p + sprintf(p, "%lld", args[i].value.int_v);
            break;
        case dDOUBLE:
            sprintf(b, "%.8f", args[i].value.dbl_v);
            if (strchr(b, '.') == NULL)
                strcat(b, ".0");
            p = p + sprintf(p, "%s", b);
            break;
        case dTSTAMP:
            q = timestamp_to_string(args[i].value.tstamp_v);
            p = p + sprintf(p, "%s", q);
            free(q);
            break;
        case dIDENT:
            if (type == dIDENT)
                break;
        case dSTRING:
            p = p + sprintf(p, "%s", args[i].value.str_v);
            break;
        }
        if (!(args[i].flags & NOTASSIGN))
            freeDSE(&args[i]);
    }
    return strdup(buf);
}

static void lookup(int lineno, DataStackEntry *table, char *id, DataStackEntry *d) {
    DataStackEntry *p;
    if (iflog) fprintf(stderr, "lookup entered.\n");
    if (hm_get((table->value.map_v)->hm, id, (void **)&p))
        *d = *p;
    else		/* id not defined, execerror */
        execerror(lineno, id, " not mapped to a value");
}

#define DEFAULT_SEQ_INCR 10

static void appendSequence(int lineno, GAPLSequence *s, int nargs, DataStackEntry *args) {
    int i, ndx;
    DataStackEntry *d;
    if ((s->size - s->used) < nargs) {	/* have to grow the array */
        DataStackEntry *p, *q;
        int n = s->size + DEFAULT_SEQ_INCR;
        q = (DataStackEntry *)malloc(n * sizeof(DataStackEntry));
        if (!q)
            execerror(lineno, "allocation failure appending to sequence", NULL);
        p = s->entries;
        s->entries = q;
        s->size = n;
        n = s->used;
        while (n-- > 0)
            *q++ = *p++;
    }
    ndx = s->used;
    for (i = 0, d = args; i < nargs; i++, d++) {
        int t = d->type;
        DataStackEntry dse;
        if (t != dBOOLEAN && t != dINTEGER && t != dDOUBLE &&
                t != dSTRING && t != dTSTAMP)
            execerror(lineno, "append: ", "restricted to basic types for a sequence");
        dse = *d;
        dse.flags = 0;
        if (t == dSTRING && (d->flags & DUPLICATE)) {
            dse.value.str_v = strdup(d->value.str_v);
            dse.flags = MUST_FREE;
        }
        s->entries[ndx++] = dse;
    }
    s->used = ndx;
}

static void insert(int lineno, DataStackEntry *table, char *id, DataStackEntry *datum) {
    void *olddatum;
    DataStackEntry *d;
    int t;
    if (iflog) fprintf(stderr, "insert entered.\n");
    if (table->type != dMAP)
        execerror(lineno, "first argument to insert() must be a map", NULL);
    t = (table->value.map_v)->type;
    if (t != datum->type)
        execerror(lineno, "Attempt to insert incorrect type into map", NULL);
    d = dse_duplicate(*datum);
    if (d->flags & DUPLICATE) {
        d->value.str_v = strdup(datum->value.str_v);
        d->flags = MUST_FREE;
    } else
        d->flags = 0;
    if (hm_put((table->value.map_v)->hm, id, d, &olddatum) && olddatum) {
        DataStackEntry *dse = (DataStackEntry *)olddatum;
        //if (dse->type != dSEQUENCE && dse->type != dWINDOW)
        freeDSE(dse);
        // dse_free(dse);

    }
}

static void frequentItems(int lineno, DataStackEntry *table, char *id, long long k) {
    DataStackEntry *p = NULL;
    int t;
    void *old;
    t = (table->value.map_v)->type;
    if (t != dINTEGER)
        execerror(lineno, "Attempt to find frequent items in a non-integer map", NULL);
    if (hm_get((table->value.map_v)->hm, id, (void **)&p)) {
        /* increase count */
        p->value.int_v += 1;
    } else if (hm_size((table->value.map_v)->hm) < k-1) {
        /* insert */
        DataStackEntry d;
        d.type = dINTEGER;
        d.flags = 0;
        d.value.int_v = 1;
        (void)hm_put((table->value.map_v)->hm, id, dse_duplicate(d), &old);
        if (old) {
            execerror(lineno, "A frequent item was replaced", NULL);
        }
    } else {
        /* iterate and decrease count; if count is 0, delete */
        char **keys;
        long i, n;
        void *datum;
        keys = hm_keyArray((table->value.map_v)->hm, &n);
        for (i = 0; i < n; i++) {
            (void)hm_get((table->value.map_v)->hm, keys[i],
                         (void **)&p);
            p->value.int_v = p->value.int_v - 1;
            if (p->value.int_v == 0) {
                DataStackEntry *dx;
                (void) hm_remove((table->value.map_v)->hm, keys[i], &datum);
                dx = (DataStackEntry *)datum;
                freeDSE(dx);
                dse_free(dx);
            }
        }
        free(keys);
    }
}

static GAPLSequence *lsqrfit(int lineno, GAPLWindow *w) {
    GAPLSequence s;
    DataStackEntry *q;
    DataStackEntry dsea, dseb;
    Iterator *it;
    GAPLWindowEntry *we;
    unsigned long long first;

    double N = 0.0;
    double X = 0.0;
    double Y = 0.0;
    double XY= 0.0;
    double X2= 0.0;

    double x, y, a, b;

    if (iflog) fprintf(stderr, "lsqrfit entered.\n");
    if (w->dtype != dINTEGER && w->dtype != dDOUBLE)
        execerror(lineno, "least squares fit only legal on windows of ints or reals", NULL);
    if (w->wtype != dSECS)
        execerror(lineno, "least squares fit only legal on time-based windows", NULL);

    it = ll_it_create(w->ll);
    while (it_hasNext(it)) {
        (void)it_next(it, (void **)&we);
        if (N == 0.0)
        {
            first = we->tstamp;
        }
        x = (double)(we->tstamp - first);
        if (w->dtype == dINTEGER)
            y =(double)((we->dse).value.int_v);
        else
            y = (we->dse).value.dbl_v;
        // printf("[alex] x=%10.10f\ty=%10.10f\n", x, y);
        X += x;
        Y += y;
        X2+= x*x;
        XY+= x*y;
        N++;
    }
    it_destroy(it);
    if (N > 1.0) {
        a = ( (N*XY) - (X* Y) ) / ( (N*X2) - (X*X) );
        b = ( (Y*X2) - (X*XY) ) / ( (N*X2) - (X*X) );
    } else {
        a = 0.0;
        b = 0.0;
    }
    // printf("[alex] a=%10.10f\tb=%10.10f\n", a, b);
    dsea.type = dDOUBLE;
    dsea.flags = 0;
    dsea.value.dbl_v = a;
    dseb.type = dDOUBLE;
    dseb.flags = 0;
    dseb.value.dbl_v = b;
    q = (DataStackEntry *)malloc(2 * sizeof(DataStackEntry));
    if (! q)
        execerror(lineno, "allocation failure in lsqrf()", NULL);
    s.size = 2;
    s.used = 0;
    s.entries = q;
    appendSequence(lineno, &s, 1, &dsea);
    appendSequence(lineno, &s, 1, &dseb);
    return seq_duplicate(s);
}

#define DEFAULT_SEQ_SIZE 10

static GAPLSequence *genSequence(int lineno, long long nargs, DataStackEntry *args) {
    GAPLSequence s;
    DataStackEntry *q;
    int n = (nargs > 0) ? nargs : DEFAULT_SEQ_SIZE;
    q = (DataStackEntry *)malloc(n * sizeof(DataStackEntry));
    if (! q)
        execerror(lineno, "allocation failure in Sequence()", NULL);
    s.size = n;
    s.used = 0;
    s.entries = q;
    appendSequence(lineno, &s, nargs, args);
    return seq_duplicate(s);
}

static void appendWindow(int lineno, GAPLWindow *w, DataStackEntry *d, DataStackEntry *ts) {
    GAPLWindowEntry we;
    tstamp_t first;
    int n;
    if (iflog) fprintf(stderr, "appendWindow entered.\n");
    if (w->dtype != d->type)
        execerror(lineno, "type mismatch for window and value to append", NULL);
    if (w->dtype == dSEQUENCE && ll_size(w->ll) > 0L) {
        Iterator *it = ll_it_create(w->ll);
        GAPLWindowEntry *p;
        GAPLSequence *s;
        GAPLSequence *u = (GAPLSequence *) d->value.seq_v;
        (void)it_next(it, (void **)&p);
        it_destroy(it);
        s = (GAPLSequence *) p->dse.value.seq_v;
        if (s->used != u->used)
            execerror(lineno, "type mismatch for window and value to append", NULL);
        else {
            int j;
            for (j = 0; j < s->used; j++) {
                if (s->entries[j].type != u->entries[j].type)
                    execerror(lineno, "type mismatch for window and value to append", NULL);
            }
        }
    }
    if (w->dtype == dSEQUENCE) {
        we.dse.flags = d->flags;
        we.dse.type = d->type;
        GAPLSequence *u = (GAPLSequence *) d->value.seq_v;
        we.dse.value.seq_v = genSequence(lineno, (long long) u->size, u->entries);
        /*printf("[alex] add ");
        printDSE(&we.dse, stdout);
        printf("\n");*/
    } else {
        we.dse = *d;
    }

    if (we.dse.flags & DUPLICATE) {
        we.dse.value.str_v = strdup(d->value.str_v);
        we.dse.flags = MUST_FREE;
    } else
        we.dse.flags = 0;
    if (w->wtype == dSECS)
        we.tstamp = ts->value.tstamp_v;
    (void) ll_addFirst(w->ll, we_duplicate(we));
    n = 0;
    switch(w->wtype) {
    case dROWS:
        if (ll_size(w->ll) > (long)w->rows_secs) {
            n++;
        }
        break;
    case dSECS: {
        Iterator *it = ll_it_create(w->ll);
        GAPLWindowEntry *p;
        first = timestamp_sub_incr(we.tstamp, (unsigned long)(w->rows_secs), 0);
        while (it_hasNext(it)) {
            (void)it_next(it, (void **)&p);
            if (p->tstamp > first)
                break;
            n++;
        }
        it_destroy(it);
        break;
    }
    }
    while (n-- >0) {
        GAPLWindowEntry *entry;
        (void) ll_removeFirst(w->ll, (void **)&entry);
        freeDSE(&entry->dse);
        free(entry);
    }
}

static double maximum(int lineno, GAPLWindow *w) {
    Iterator *it;
    GAPLWindowEntry *we;
    double max = 0.0;
    if (iflog) fprintf(stderr, "maximum entered.\n");
    if (w->dtype != dDOUBLE)
        execerror(lineno, "maximum only legal on windows of positive reals", NULL);
    it = ll_it_create(w->ll);
    while (it_hasNext(it)) {
        (void)it_next(it, (void **)&we);
        double x;
        x = (we->dse).value.dbl_v;
        if (max < x) max = x;
    }
    it_destroy(it);
    return max;
}

static double average(int lineno, GAPLWindow *w) {
    Iterator *it;
    GAPLWindowEntry *we;
    int N = 0;
    double sum = 0.0;
    if (iflog) fprintf(stderr, "average entered.\n");
    if (w->dtype != dINTEGER && w->dtype != dDOUBLE)
        execerror(lineno, "average only legal on windows of ints or reals", NULL);
    it = ll_it_create(w->ll);
    while (it_hasNext(it)) {
        (void)it_next(it, (void **)&we);
        double x;
        if (w->dtype == dINTEGER)
            x =(double)((we->dse).value.int_v);
        else
            x = (we->dse).value.dbl_v;
        sum += x;
        N++;
    }
    it_destroy(it);
    if (N == 0)
        return 0.0;
    else
        return (sum/(double)N);
}

static double std_dev(int lineno, GAPLWindow *w) {
    Iterator *it;
    GAPLWindowEntry *we;
    int N = 0;
    double sum = 0.0;
    double sumsq = 0.0;
    if (iflog) fprintf(stderr, "std_dev entered.\n");
    if (w->dtype != dINTEGER && w->dtype != dDOUBLE)
        execerror(lineno, "average only legal on windows of ints or reals", NULL);
    it = ll_it_create(w->ll);
    while (it_hasNext(it)) {
        (void)it_next(it, (void **)&we);
        double x;
        if (w->dtype == dINTEGER)
            x =(double)((we->dse).value.int_v);
        else
            x = (we->dse).value.dbl_v;
        sum += x;
        sumsq += x * x;
        N++;
    }
    it_destroy(it);
    if (N == 0 || N == 1)
        return 0.0;
    else
        return sqrt((sumsq - sum * sum / (double) N) / (double)(N-1));
}



static void sendevent(MachineContext *mc, long long nargs, DataStackEntry *args) {
    int i, n, types[50];
    char event[SOCK_RECV_BUF_LEN], *p;
    Q_Decl(result,SOCK_RECV_BUF_LEN);
    DataStackEntry d;

    if (iflog) logit("sendevent entered.\n", mc->au);
    n = 0;
    p = event;
    if (args[0].type != dTSTAMP && args[0].type != dEVENT) {
        types[n++] = dTSTAMP;
        d.type = dTSTAMP;
        d.value.tstamp_v = timestamp_now();
        p += packvalue(d, p);
        p += sprintf(p, "<|>");
    }
    for (i = 0; i < nargs; i++) {
        if (args[i].type == dSEQUENCE) {
            int j;
            GAPLSequence *s = args[i].value.seq_v;
            for (j = 0; j < s->used; j++) {
                types[n++] = s->entries[j].type;
                p += packvalue(s->entries[j], p);
                p += sprintf(p, "<|>");
            }
        } else if (args[i].type == dEVENT) {
            int j;
            DataStackEntry *dse;
            Event *ev = (Event *)args[i].value.ev_v;
            int m = ev_theData(ev, &dse);
            for (j = 0; j < m; j++) {
                types[n++] = dse[j].type;
                p += packvalue(dse[j], p);
                p += sprintf(p, "<|>");
            }
        } else {
            types[n++] = args[i].type;
            p += packvalue(args[i], p);
            p += sprintf(p, "<|>");
        }
        if (!(args[i].type == dEVENT || (args[i].flags & NOTASSIGN)))
            freeDSE(&args[i]);
    }
    p = result;
    p += sprintf(p, "0<|>%lu<|>%d<|>1<|>\n", au_id(mc->au), n);
    for (i = 0; i < n; i++) {
        p += sprintf(p, "c%d:", i);
        switch(types[i]) {
        case dBOOLEAN:
            p += sprintf(p, "boolean");
            break;
        case dINTEGER:
            p += sprintf(p, "integer");
            break;
        case dDOUBLE:
            p += sprintf(p, "real");
            break;
        case dTSTAMP:
            p += sprintf(p, "timestamp");
            break;
        case dSTRING:
            p += sprintf(p, "varchar");
            break;
        }
        p += sprintf(p, "<|>");
    }
    p += sprintf(p, "\n%s\n", event);
    if (! au_rpc(mc->au)) {
        printf("%s", result);
        fflush(stdout);
    } else {
        char resp[100];
        unsigned rlen, len = strlen(result) + 1;
        if (! rpc_call(au_rpc(mc->au), Q_Arg(result), len, resp, sizeof(resp), &rlen))
            execerror(mc->pc->lineno, "callback RPC failed", NULL);
    }
}

/* send window ------------- */

// #define QUERY_SIZE 61439
#define QUERY_SIZE 32767

/*
static void sendwindow(MachineContext *mc, Window *W) {

	int i;
	int n;
	int f;

	unsigned long size;

	int types[50];

	char event[SOCK_RECV_BUF_LEN];

	char *p;

	int rows, length = 0;

	Q_Decl(result, SOCK_RECV_BUF_LEN);

	memset(event, 0, SOCK_RECV_BUF_LEN);
	memset(result, 0, SOCK_RECV_BUF_LEN);

	if (iflog)
		logit("sendwindow entered.\n", mc->au);

	size = ll_length(W->ll);
	printf("[alex] sending window of %lu elements.\n", size);

	if (size == 0)
		execerror(mc->pc->lineno, "Error sending an empty window", NULL);

	n = 0;
	f = 0;

	p = event;

	LLIterator *it = ll_iter_create(W->ll);
	WindowEntry *w;

	rows = 0;
	length = 0;
	while ((w = (WindowEntry *)ll_iter_next(it))) {

		if (W->dtype == dSEQUENCE) {
      		Sequence *s = (Sequence *) w->dse.value.seq_v;
			for (i = 0; i < s->used; i++) {
            	if (! f)
					types[n++] = s->entries[i].type;
            	length += packvalue(s->entries[i], event + length);
            	length += sprintf(event + length, "<|>");
         	}
		} else { // type is either `int' or `real'
			if (! f) types[n++] = w->dse.type;
			length += packvalue(w->dse, event + length);
			length += sprintf(event + length, "<|>");
		}
		length += sprintf(event + length, "\n");
		// Row accumulated.
		rows++;
		printf("[alex] %d rows (%d bytes) accumulated\n", rows, length);
		if (length > QUERY_SIZE) {
			printf("[alex] %d > QUERY_SIZE.\n", length);
			break;
		}

		f = 1;
	}
	ll_iter_delete(it);
	p = result;
	// p += sprintf(p, "0<|>%lu<|>%d<|>%lu<|>\n", au_id(mc->au), n, size);
	p += sprintf(p, "0<|>%lu<|>%d<|>%d<|>\n", au_id(mc->au), n, rows);
	for (i = 0; i < n; i++) {
		p += sprintf(p, "c%d:", i);
		switch(types[i]) {
			case dBOOLEAN: p += sprintf(p, "boolean"); break;
			case dINTEGER: p += sprintf(p, "integer"); break;
			case dDOUBLE: p += sprintf(p, "real"); break;
			case dTSTAMP: p += sprintf(p, "timestamp"); break;
			case dSTRING: p += sprintf(p, "varchar"); break;
		}
		p += sprintf(p, "<|>");
	}
	p += sprintf(p, "\n%s", event);
	if (! au_rpc(mc->au)) {
		printf("%s", result);
		fflush(stdout);
	} else {
		char resp[1000];
		unsigned rlen, len = strlen(result) + 1;
		if (! rpc_call(au_rpc(mc->au), Q_Arg(result), len, resp, sizeof(resp), &rlen))
			execerror(mc->pc->lineno, "callback RPC failed", NULL);
	}
    printf("[alex] window sent.\n");
	return;
}
*/

static void sendwindow(MachineContext *mc, GAPLWindow *W) {

    // unsigned long long start;
    int i;
    int n;
    int f;
    long size;
    int types[50];
    char event[SOCK_RECV_BUF_LEN];
    char *p;
    int rows, length = 0;
    Q_Decl(result, SOCK_RECV_BUF_LEN);

    memset(event,  0, SOCK_RECV_BUF_LEN);
    memset(result, 0, SOCK_RECV_BUF_LEN);

    if (iflog)
        logit("sendwindow entered.\n", mc->au);

    size = ll_size(W->ll);
    // printf("[alex] sending window of %lu elements.\n", size);

    if (size == 0)
        execerror(mc->pc->lineno, "Error sending an empty window", NULL);

    n = 0;
    f = 0;

    rows = 0;
    length = 0;

    Iterator *it = ll_it_create(W->ll);
    GAPLWindowEntry *w;

    while (it_hasNext(it)) {
        (void)it_next(it, (void **)&w);

        if (W->dtype == dSEQUENCE) {
            GAPLSequence *s = (GAPLSequence *) w->dse.value.seq_v;
            for (i = 0; i < s->used; i++) {
                if (! f)
                    types[n++] = s->entries[i].type;
                length += packvalue(s->entries[i], event + length);
                length += sprintf(event + length, "<|>");
            }
        } else { /* type is either `int' or `real' */
            if (! f) types[n++] = w->dse.type;
            length += packvalue(w->dse, event + length);
            length += sprintf(event + length, "<|>");
        }
        length += sprintf(event + length, "\n");
        /* Row accumulated. */
        rows++;
        // printf("[alex] %d rows (%d bytes) accumulated\n", rows, length);

        f = 1; /* First row is processed. */

        if (length > QUERY_SIZE) {
            // printf("[alex] %d > QUERY_SIZE.\n", length);
            memset(result, 0, SOCK_RECV_BUF_LEN);
            p = result;
            p += sprintf(p, "0<|>%lu<|>%d<|>%d<|>\n", au_id(mc->au), n, rows);
            for (i = 0; i < n; i++) {
                p += sprintf(p, "c%d:", i);
                switch(types[i]) {
                case dBOOLEAN:
                    p += sprintf(p, "boolean");
                    break;
                case dINTEGER:
                    p += sprintf(p, "integer");
                    break;
                case dDOUBLE:
                    p += sprintf(p, "real");
                    break;
                case dTSTAMP:
                    p += sprintf(p, "timestamp");
                    break;
                case dSTRING:
                    p += sprintf(p, "varchar");
                    break;
                }
                p += sprintf(p, "<|>");
            }
            p += sprintf(p, "\n%s", event);
            if (! au_rpc(mc->au)) {
                printf("%s", result);
                fflush(stdout);
            } else {
                // start = timestamp_now();
                char resp[1000];
                unsigned rlen, len = strlen(result) + 1;
                if (! rpc_call(au_rpc(mc->au), Q_Arg(result), len, resp, sizeof(resp), &rlen))
                    execerror(mc->pc->lineno, "callback RPC failed", NULL);
                // printf("[alex] %llu\n", (timestamp_now() - start));
            }

            length = 0;
            memset(event, 0, SOCK_RECV_BUF_LEN);
            memset(result, 0, SOCK_RECV_BUF_LEN);
            rows = 0;
        }
    }
    it_destroy(it);
    /* All elements processed have been processed at this point. */
    if (length > 0 && rows > 0) {
        p = result;
        p += sprintf(p, "0<|>%lu<|>%d<|>%d<|>\n", au_id(mc->au), n, rows);
        for (i = 0; i < n; i++) {
            p += sprintf(p, "c%d:", i);
            switch(types[i]) {
            case dBOOLEAN:
                p += sprintf(p, "boolean");
                break;
            case dINTEGER:
                p += sprintf(p, "integer");
                break;
            case dDOUBLE:
                p += sprintf(p, "real");
                break;
            case dTSTAMP:
                p += sprintf(p, "timestamp");
                break;
            case dSTRING:
                p += sprintf(p, "varchar");
                break;
            }
            p += sprintf(p, "<|>");
        }
        p += sprintf(p, "\n%s", event);
        if (! au_rpc(mc->au)) {
            printf("%s", result);
            fflush(stdout);
        } else {
            char resp[1000];
            unsigned rlen, len = strlen(result) + 1;
            if (! rpc_call(au_rpc(mc->au), Q_Arg(result), len, resp, sizeof(resp), &rlen))
                execerror(mc->pc->lineno, "callback RPC failed", NULL);
        }
    }
    // printf("[alex] window sent.\n");
    return;
}

/* --------------------------*/

static int *column_type(DataStackEntry dse) {
    int *ans = NULL;

    switch (dse.type) {
    case dBOOLEAN:
        ans = PRIMTYPE_BOOLEAN;
        break;
    case dINTEGER:
        ans = PRIMTYPE_INTEGER;
        break;
    case dDOUBLE:
        ans = PRIMTYPE_REAL;
        break;
    case dSTRING:
        ans = PRIMTYPE_VARCHAR;
        break;
    case dTSTAMP:
        ans = PRIMTYPE_TIMESTAMP;
        break;
    }
    return ans;
}

#define NCOLUMNS 20

static void publishevent(MachineContext *mc, long long nargs,
                         DataStackEntry *args) {
    int i, n, ans, error;
    sqlinsert sqli;
    char *colval[NCOLUMNS];
    int *coltype[NCOLUMNS];
    char buf[2048];

    if (iflog) logit("publishevent entered.\n", mc->au);
    if (args[0].type != dSTRING)
        execerror(mc->pc->lineno, "Incorrect first argument in call to publish", NULL);
    else if (! top_exist(args[0].value.str_v))
        execerror(mc->pc->lineno, "topic does not exist :", args[0].value.str_v);
    sqli.tablename = args[0].value.str_v;
    sqli.transform = 0;
    n = -1;
    error = 0;
    for (i = 1; i < nargs; i++) {
        if (args[i].type == dSEQUENCE) {
            int j;
            GAPLSequence *s = args[i].value.seq_v;
            for (j = 0; j < s->used; j++) {
                if (++n >= NCOLUMNS) {
                    error++;
                    goto done;
                }
                (void) packvalue(s->entries[j], buf);
                colval[n] = strdup(buf);
                coltype[n] = column_type(s->entries[j]);
            }
        } else {
            if (++n >= NCOLUMNS) {
                error++;
                goto done;
            }
            (void) packvalue(args[i], buf);
            colval[n] = strdup(buf);
            coltype[n] = column_type(args[i]);
        }
        if (!(args[i].flags & NOTASSIGN))
            freeDSE(&args[i]);
    }
done:
    if (! error) {
        n++;
        sqli.colval = colval;
        sqli.coltype = coltype;
        sqli.ncols = n;
        ans = hwdb_insert(&sqli);
    }
    for (i = 0; i < n; i++)
        free(colval[i]);
    if (error)
        execerror(mc->pc->lineno, "Too many arguments for topic: ", sqli.tablename);
    if (! ans)
        execerror(mc->pc->lineno, "Error publishing event to topic: ", sqli.tablename);
}

static void doremove(DataStackEntry *table, char *id) {
    void *olddatum;
    if (iflog) fprintf(stderr, "remove entered.\n");
    if (hm_remove((table->value.map_v)->hm, id, &olddatum) && olddatum) {
        DataStackEntry *d = (DataStackEntry *)olddatum;
        if (d->type != dSEQUENCE && d->type != dWINDOW)
            freeDSE(d);
        dse_free(olddatum);
    }
}

void procedure(MachineContext *mc) {
    DataStackEntry name = mc->pc->u.immediate;
    DataStackEntry narg = (mc->pc+1)->u.immediate;
    DataStackEntry args[MAX_ARGS];
    int i;
    unsigned int nargs;
    struct fpargs *range;

    nargs = narg.value.int_v;
    (void)hm_get(builtins, name.value.str_v, (void **)&range);
    for (i = narg.value.int_v -1; i >= 0; i--)
        args[i] = pop(mc->stack);
    if (nargs < range->min || nargs > range->max)
        execerror(mc->pc->lineno, "Incorrect # of arguments for procedure", name.value.str_v);
    if (iflog) {
        fprintf(stderr, "%08lx: %s(", au_id(mc->au), name.value.str_v);
        for (i = 0; i < narg.value.int_v; i++) {
            if (i > 0)
                fprintf(stderr, ", ");
            dumpDataStackEntry(args+i, 0);
        }
        fprintf(stderr, ")\n");
    }
    switch(range->index) {
    case 0: {		/* void topOfHeap() */
        //mem_heap_end_address("Top of heap: ");
        break;
    }
    case 1: {		/* void insert(map, ident, map.type) */
        /* args[0] is map, args[1] is ident, args[2] is new value */
        if (args[0].type == dMAP) {
            insert(mc->pc->lineno, args+0, args[1].value.str_v, args+2);
        } else if (args[0].type == dPTABLE) {
            if (! ptab_update(args[0].value.str_v, args[1].value.str_v, args[2].value.seq_v))
                execerror(mc->pc->lineno, args[0].value.str_v, " cannot update");
        } else
            execerror(mc->pc->lineno, "insert invoked on non-map", NULL);
        if (!(args[1].flags & NOTASSIGN))
            freeDSE(args+1);
        break;
    }
    case 2: {		/* void remove(map, ident) */
        /* args[0] is map, args[1] is ident */
        if (args[0].type == dMAP && args[1].type == dIDENT) {
            doremove(args+0, args[1].value.str_v);
        /* args[0] is ptable, args[1] is ident */
        } else if (args[0].type == dPTABLE && args[1].type == dIDENT) {
            ptab_delete(args[0].value.str_v, args[1].value.str_v);
        } else {
            execerror(mc->pc->lineno, "incorrect data types in call to remove()", NULL);
        }
        if (!(args[1].flags & NOTASSIGN))
            freeDSE(args+1);
        break;
    }
    case 3: {		/* void send(arg, ...) [max 20 args] */
        if (args[0].type == dWINDOW) {
            // printf("[alex] send window\n");
            if (narg.value.int_v != 1)
                execerror(mc->pc->lineno,
                          "incorrect number of arguments in call to send()", NULL);
            sendwindow(mc, args[0].value.win_v);
        } else {
            // printf("[alex] send event\n");
            sendevent(mc, narg.value.int_v, args);
        }
        break;
    }
    case 4: {		/* void append(window, window.dtype[, tstamp]); */
        /* void append(sequence, arg[, ...]) */
        /*
         * two arguments are required for ROWS limited windows
         * the tstamp third argument is required for SECS limited windows
         * for sequences, one can specify as many arguments as one wants
         */
        if (args[0].type != dWINDOW && args[0].type != dSEQUENCE)
            execerror(mc->pc->lineno, "append: ", "only legal for windows and sequences");
        if (args[0].type == dWINDOW) {
            GAPLWindow *w = args[0].value.win_v;
            if(w->wtype == dROWS && narg.value.int_v != 2)
                execerror(mc->pc->lineno, "append: ", "ROW limited windows require 2 arguments");
            else if (w->wtype == dSECS && narg.value.int_v != 3)
                execerror(mc->pc->lineno, "append: ", "SEC limited windows require 3 arguments");
            appendWindow(mc->pc->lineno, w, args+1, args+2);
        } else {
            GAPLSequence *s = args[0].value.seq_v;
            appendSequence(mc->pc->lineno, s, nargs-1, args+1);
        }
        break;
    }
    case 5: {		/* void publish(topic, arg, ...) [max 20 args] */
        publishevent(mc, narg.value.int_v, args);
        break;
    }
    case 6: { /* void frequent(map, ident, k) */
        if (args[0].type != dMAP || args[1].type != dIDENT || args[2].type != dINTEGER)
            execerror(mc->pc->lineno, "incorrect data types in call to frequent()", NULL);
        frequentItems(mc->pc->lineno, args+0, args[1].value.str_v, args[2].value.int_v);
        break;
    }
    }
    mc->pc = mc->pc + 2;
}

enum ts_field { SEC_IN_MIN, MIN_IN_HOUR, HOUR_IN_DAY, DAY_IN_MONTH,
                MONTH_IN_YEAR, YEAR_IN, DAY_IN_WEEK
              };

static long long ts_field_value(enum ts_field field, tstamp_t ts) {
    struct tm result;
    time_t secs;
    unsigned long long nsecs;
    long long ans = 0;

    nsecs = ts / 1000000000;
    secs = nsecs;
    (void) localtime_r(&secs, &result);
    switch(field) {
    case SEC_IN_MIN:
        ans = (long long)(result.tm_sec);
        break;
    case MIN_IN_HOUR:
        ans = (long long)(result.tm_min);
        break;
    case HOUR_IN_DAY:
        ans = (long long)(result.tm_hour);
        break;
    case DAY_IN_MONTH:
        ans = (long long)(result.tm_mday);
        break;
    case MONTH_IN_YEAR:
        ans = (long long)(1 + result.tm_mon);
        break;
    case YEAR_IN:
        ans = (long long)(1900 + result.tm_year);
        break;
    case DAY_IN_WEEK:
        ans = (long long)(result.tm_wday);
        break;
    }
    return ans;
}

static GAPLIterator *genIterator(int lineno, DataStackEntry d) {
    GAPLIterator it;
    long n;

    if (d.type == dMAP || d.type == dPTABLE) {
        char **keys;
        if (d.type == dMAP) {
            HashMap *hm = d.value.map_v->hm;
            keys = hm_keyArray(hm, &n);
            it.type = dMAP;
        } else {
            n = ptab_keys(d.value.str_v, &keys);
            it.type = dPTABLE;
        }
        if (n < 0)
            execerror(lineno, "memory allocation failure generating iterator", NULL);
        it.u.m_idents = keys;
    } else {
        GAPLWindow *w = d.value.win_v;
        Iterator *iter = ll_it_create(w->ll);
        if (! iter)
            execerror(lineno, "memory allocation failure generating iterator", NULL);
        n = ll_size(w->ll);
        it.type = dWINDOW;
        it.dtype = w->dtype;
        it.u.w_it = iter;
    }
    it.next = 0;
    it.size = n;
    return iter_duplicate(it);
}

static int hasEntry(DataStackEntry *d, char *key) {
    if (d->type == dMAP) {
        DataStackEntry *dummy;
        if (hm_get((d->value.map_v)->hm, key, (void **)&dummy))
            return 1;
        else
            return 0;
    } else {
        return ptab_hasEntry(d->value.str_v, key);
    }
}

static int hasNext(GAPLIterator *it) {
    return (it->next < it->size);
}

static DataStackEntry nextElement(int lineno, GAPLIterator *it) {
    DataStackEntry d;

    if (it->next >= it->size)
        execerror(lineno, "next() invoked on exhausted iterator", NULL);
    if (it->type == dMAP || it->type == dPTABLE) {
        int n = it->next++;
        d.value.str_v = it->u.m_idents[n];
        d.type = dIDENT;
    } else {                        /* it's a window */
        GAPLWindowEntry *we;
        (void)it_next(it->u.w_it, (void **)&we);
        it->next++;
        d = we->dse;
    }
    d.flags = 0;
    return d;
}

static DataStackEntry seqElement(int lineno, GAPLSequence *seq, long long ndx) {
    if (ndx >= seq->used)
        execerror(lineno, "seqElement(): index out of range", NULL);
    return (seq->entries[ndx]);
}

static DataStackEntry winElement(int lineno, GAPLWindow *w, long long ndx) {
    long long count = 0;
    GAPLWindowEntry *we = NULL;
    Iterator *it;
    if (ndx >= (long long) ll_size(w->ll))
        execerror(lineno, "winElement(): index out of range", NULL);
    it = ll_it_create(w->ll);
    count = 0;
    while (it_hasNext(it)) {
        (void)it_next(it, (void **)&we);
        if (ndx == count)
            break;
        count += 1;
    }
    it_destroy(it);
    if (! we)
        execerror(lineno, "winElement(): element in NULL", NULL);
    return we->dse;
}

static long long IP4Addr(int lineno, char *addr) {
    struct in_addr in;
    long long ipaddr;

    if (! inet_aton(addr, &in))
        execerror(lineno, "Invalid IP address", addr);
    ipaddr = ntohl(in.s_addr);
    return (ipaddr & 0xffffffff);
}

static long long IP4Mask(long long slashN) {
    long long mask;
    int i;

    mask = 0;
    for (i = 0; i < slashN; i++)
        mask = (mask << 1) | 1;
    mask <<= (32 - slashN);
    return mask;
}

static int matchNetwork(int lineno, char *str, long long mask, long long subnet) {
    long long addr = IP4Addr(lineno, str);
    return ((addr & mask) == subnet);
}

void function(MachineContext *mc) {
    DataStackEntry name = mc->pc->u.immediate;
    DataStackEntry narg = (mc->pc+1)->u.immediate;
    DataStackEntry args[MAX_ARGS], d;
    int i;
    unsigned int nargs;
    struct fpargs *range;

    nargs = narg.value.int_v;
    (void)hm_get(builtins, name.value.str_v, (void **)&range);
    for (i = narg.value.int_v -1; i >= 0; i--)
        args[i] = pop(mc->stack);
    if (nargs < range->min || nargs > range->max)
        execerror(mc->pc->lineno, "Incorrect # of arguments for function", name.value.str_v);
    if (iflog) {
        fprintf(stderr, "%08lx: %s(", au_id(mc->au), name.value.str_v);
        for (i = 0; i < narg.value.int_v; i++) {
            if (i > 0)
                fprintf(stderr, ", ");
            dumpDataStackEntry(args+i, 0);
        }
        fprintf(stderr, ")\n");
    }
    switch(range->index) {
    case 0: {		/* real float(int) */
        d.type = dDOUBLE;
        d.flags = 0;
        if (args[0].type == dINTEGER)
            d.value.dbl_v = (double)(args[0].value.int_v);
        else if (args[0].type == dTSTAMP)
            d.value.dbl_v = (double)(args[0].value.tstamp_v);
        else
            execerror(mc->pc->lineno, "argument to float must be an int", NULL);
        break;
    }
    case 1: {		/* identifier Identifier(arg, ...) [max 20 args] */
        d.type = dIDENT;
        d.flags = MUST_FREE;
        d.value.str_v = concat(dIDENT, narg.value.int_v, args);
        break;
    }
    case 2: {		/* map.type lookup(map, identifier) */
        /* args[0] is map, args[1] is ident to search for */
        if (args[0].type == dMAP) {
            lookup(mc->pc->lineno, args+0, args[1].value.str_v, &d);
            d.flags = 0;
        } else if (args[0].type == dPTABLE) {
            GAPLSequence *s = ptab_lookup(args[0].value.str_v, args[1].value.str_v);
            if (s) {
                d.type = dSEQUENCE;
                d.flags = MUST_FREE;
                d.value.seq_v = s;
            } else
                execerror(mc->pc->lineno, args[1].value.str_v, " not mapped to a value");
        } else
            execerror(mc->pc->lineno, "attempted lookup on non-map", NULL);
        if (!(args[1].flags & NOTASSIGN))
            freeDSE(args+1);
        break;
    }
    case 3: {		/* real average(window) */
        if (args[0].type != dWINDOW)
            execerror(mc->pc->lineno, "attempt to compute average of a non-window", NULL);
        d.type = dDOUBLE;
        d.flags = 0;
        d.value.dbl_v = average(mc->pc->lineno, args[0].value.win_v);
        break;
    }
    case 4: {		/* real stdDev(window) */
        if (args[0].type != dWINDOW)
            execerror(mc->pc->lineno, "attempt to compute std deviation of a non-window", NULL);
        d.type = dDOUBLE;
        d.flags = 0;
        d.value.dbl_v = std_dev(mc->pc->lineno, args[0].value.win_v);
        break;
    }
    case 5: {		/* string currentTopic() */
        d.type = dSTRING;
        d.flags = 0;
        d.value.str_v = mc->currentTopic;
        break;
    }
    case 6: {		/* iterator Iterator(map|win|seq) */
        if (args[0].type != dMAP && args[0].type != dPTABLE && args[0].type != dWINDOW)
            execerror(mc->pc->lineno, "incorrectly typed argument to Iterator()", NULL);
        d.type = dITERATOR;
        d.flags = 0;
        d.value.iter_v = genIterator(mc->pc->lineno, args[0]);
        break;
    }
    case 7: {		/* identifier|data next(iterator) */
        if (args[0].type != dITERATOR)
            execerror(mc->pc->lineno, "incorrectly typed argument to next()", NULL);
        d = nextElement(mc->pc->lineno, args[0].value.iter_v);
        break;
    }
    case 8: {		/* tstamp tstampNow() */
        d.type = dTSTAMP;
        d.flags = 0;
        d.value.tstamp_v = timestamp_now();
        break;
    }
    case 9: {		/* tstamp tstampDelta(tstamp, int, bool) */
        long units;
        int ifmillis;
        tstamp_t ts;
        if (args[0].type != dTSTAMP || args[1].type != dINTEGER
                || args[2].type != dBOOLEAN)
            execerror(mc->pc->lineno, "incorrectly typed arguments to tstampDelta()", NULL);
        d.type = dTSTAMP;
        d.flags = 0;
        units = args[1].value.int_v;
        ifmillis = args[2].value.bool_v;
        ts = args[0].value.tstamp_v;
        if (units >= 0)
            d.value.tstamp_v = timestamp_add_incr(ts, units, ifmillis);
        else
            d.value.tstamp_v = timestamp_sub_incr(ts, -units, ifmillis);
        break;
    }
    case 10: {		/* int tstampDiff(tstamp, tstamp) */
        long long diff;
        if (args[0].type != dTSTAMP || args[1].type != dTSTAMP)
            execerror(mc->pc->lineno, "incorrectly typed arguments to tstampDiff()", NULL);
        d.type = dINTEGER;
        d.flags = 0;
        if (args[0].value.tstamp_v >= args[1].value.tstamp_v)
            diff = args[0].value.tstamp_v - args[1].value.tstamp_v;
        else
            diff = -(args[1].value.tstamp_v - args[0].value.tstamp_v);
        d.value.int_v = diff;
        break;
    }
    case 11: {		/* tstamp Timestamp(string) */
        if (args[0].type != dSTRING)
            execerror(mc->pc->lineno, "incorrectly typed argument to Timestamp()", NULL);
        d.type = dTSTAMP;
        d.flags = 0;
        d.value.tstamp_v = datestring_to_timestamp(args[0].value.str_v);
        if (!(args[0].flags & NOTASSIGN))
            freeDSE(&args[0]);
        break;
    }
    case 12: {		/* int dayInWeek(tstamp) [Sun is 0, Sat is 6] */
        if (args[0].type != dTSTAMP)
            execerror(mc->pc->lineno, "incorrectly typed argument to dayInWeek()", NULL);
        d.type = dINTEGER;
        d.flags = 0;
        d.value.int_v = ts_field_value(DAY_IN_WEEK, args[0].value.tstamp_v);
        break;
    }
    case 13: {		/* int hourInDay(tstamp) [0 .. 23] */
        if (args[0].type != dTSTAMP)
            execerror(mc->pc->lineno, "incorrectly typed argument to hourInDay()", NULL);
        d.type = dINTEGER;
        d.flags = 0;
        d.value.int_v = ts_field_value(HOUR_IN_DAY, args[0].value.tstamp_v);
        break;
    }
    case 14: {		/* int dayInMonth(tstamp) [1..31] */
        if (args[0].type != dTSTAMP)
            execerror(mc->pc->lineno, "incorrectly typed argument to dayInMonth()", NULL);
        d.type = dINTEGER;
        d.flags = 0;
        d.value.int_v = ts_field_value(DAY_IN_MONTH, args[0].value.tstamp_v);
        break;
    }
    case 15: {		/* sequence Sequence() */
        d.type = dSEQUENCE;
        d.flags = 0;
        d.value.seq_v = genSequence(mc->pc->lineno, narg.value.int_v, args);
        break;
    }
    case 16: {		/* bool hasEntry(map, identifier) */
        if (! (args[1].type == dIDENT &&
                (args[0].type == dMAP || args[0].type == dPTABLE)))
            execerror(mc->pc->lineno, "incorrectly typed arguments to hasEntry()", NULL);
        d.type = dBOOLEAN;
        d.flags = 0;
        d.value.bool_v = hasEntry(args, args[1].value.str_v);
        break;
    }
    case 17: {		/* bool hasNext(iterator) */
        if (args[0].type != dITERATOR)
            execerror(mc->pc->lineno, "incorrectly typed argument to hasNext()", NULL);
        d.type = dBOOLEAN;
        d.flags = 0;
        d.value.bool_v = hasNext(args[0].value.iter_v);
        break;
    }
    case 18: {		/* string String(arg[, ...]) */
        d.type = dSTRING;
        d.flags = MUST_FREE;
        d.value.str_v = concat(dSTRING, narg.value.int_v, args);
        break;
    }
    case 19: {		/* basictype seqElement(seq, int) */
        if (args[0].type != dSEQUENCE || args[1].type != dINTEGER)
            execerror(mc->pc->lineno, "incorrectly typed arguments to seqElement()", NULL);
        d = seqElement(mc->pc->lineno, args[0].value.seq_v, args[1].value.int_v);
        d.flags = 0;
        break;
    }
    case 20: {		/* int seqSize(seq) */
        if (args[0].type != dSEQUENCE)
            execerror(mc->pc->lineno, "incorrectly typed argument to seqSize()", NULL);
        d.type = dINTEGER;
        d.flags = 0;
        d.value.int_v = args[0].value.seq_v->used;
        break;
    }
    case 21: {		/* int IP4Addr(string) */
        if (args[0].type != dSTRING)
            execerror(mc->pc->lineno, "incorrectly typed argument to IP4Addr()", NULL);
        d.type = dINTEGER;
        d.flags = 0;
        d.value.int_v = IP4Addr(mc->pc->lineno, args[0].value.str_v);
        break;
    }
    case 22: {		/* int IP4Mask(int slashN) */
        if (args[0].type != dINTEGER)
            execerror(mc->pc->lineno, "incorrectly typed argument to IP4Mask()", NULL);
        d.type = dINTEGER;
        d.flags = 0;
        d.value.int_v = IP4Mask(args[0].value.int_v);
        break;
    }
    case 23: {		/* bool matchNetwork(string, int, int) */
        if (args[0].type != dSTRING || args[1].type != dINTEGER
                || args[2].type != dINTEGER)
            execerror(mc->pc->lineno, "incorrectly typed arguments to matchNetwork()", NULL);
        d.type = dBOOLEAN;
        d.flags = 0;
        d.value.bool_v = matchNetwork(mc->pc->lineno,
                                      args[0].value.str_v,
                                      args[1].value.int_v,
                                      args[2].value.int_v);
        break;
    }
    case 24: {		/* int secondInMinute(tstamp) [0 .. 60] */
        if (args[0].type != dTSTAMP)
            execerror(mc->pc->lineno, "incorrectly typed argument to secondInMinute()", NULL);
        d.type = dINTEGER;
        d.flags = 0;
        d.value.int_v = ts_field_value(SEC_IN_MIN, args[0].value.tstamp_v);
        break;
    }
    case 25: {		/* int minuteInHour(tstamp) [0 .. 59] */
        if (args[0].type != dTSTAMP)
            execerror(mc->pc->lineno, "incorrectly typed argument to minuteInHour()", NULL);
        d.type = dINTEGER;
        d.flags = 0;
        d.value.int_v = ts_field_value(MIN_IN_HOUR, args[0].value.tstamp_v);
        break;
    }
    case 26: {		/* int monthInYear(tstamp) [1 .. 12] */
        if (args[0].type != dTSTAMP)
            execerror(mc->pc->lineno, "incorrectly typed argument to monthInYear()", NULL);
        d.type = dINTEGER;
        d.flags = 0;
        d.value.int_v = ts_field_value(MONTH_IN_YEAR, args[0].value.tstamp_v);
        break;
    }
    case 27: {		/* int yearIn(tstamp) [1900 .. ] */
        if (args[0].type != dTSTAMP)
            execerror(mc->pc->lineno, "incorrectly typed argument to yearIn()", NULL);
        d.type = dINTEGER;
        d.flags = 0;
        d.value.int_v = ts_field_value(YEAR_IN, args[0].value.tstamp_v);
        break;
    }
    case 28: { /* real power(real, real) */
        if (args[0].type != dDOUBLE || args[1].type != dDOUBLE)
            execerror(mc->pc->lineno, "incorrectly typed arguments to power()", NULL);
        d.type = dDOUBLE;
        d.flags = 0;
        // fprintf(stderr, "x^y: %.1f^%.1f\n", args[0].value.dbl_v, args[1].value.dbl_v);
        d.value.dbl_v = pow(args[0].value.dbl_v, args[1].value.dbl_v);
        break;
    }
    case 29: { /* int winSize(win) */
        if (args[0].type != dWINDOW)
            execerror(mc->pc->lineno, "incorrectly typed argument to winSize()", NULL);
        d.type = dINTEGER;
        d.flags = 0;
        GAPLWindow *w = args[0].value.win_v;
        d.value.int_v = (int)ll_size(w->ll);
        break;
    }
    case 30: { /* sequence lsqrf(win) */
        if (args[0].type != dWINDOW)
            execerror(mc->pc->lineno, "incorrectly typed argument to lsqrf()", NULL);
        d.type = dSEQUENCE;
        d.flags = 0;
        d.value.seq_v = lsqrfit(mc->pc->lineno, args[0].value.win_v);
        break;
    }
    case 31: { /* real max(win) */
        if (args[0].type != dWINDOW)
            execerror(mc->pc->lineno, "attempt to compute maximum of a non-window", NULL);
        d.flags = 0;
        d.type = dDOUBLE;
        d.value.dbl_v = maximum(mc->pc->lineno, args[0].value.win_v);
        break;
    }
    case 32: { /* int floor(real) */
        if (args[0].type != dDOUBLE)
            execerror(mc->pc->lineno, "incorrectly typed arguments to floor()", NULL);
        d.type = dINTEGER;
        d.flags = 0;
        d.value.int_v = (signed long long) floor(args[0].value.dbl_v);
        break;
    }
    case 33: { /* int mapSize(map) */
        if (args[0].type != dMAP)
            execerror(mc->pc->lineno, "incorrectly typed argument to mapSize()", NULL);
        d.type = dINTEGER;
        d.flags = 0;
        HashMap *hm = args[0].value.map_v->hm;
        d.value.int_v = hm_size(hm); // hm->nelements;
        break;
    }
    case 34: { /* win.type winElement(win, int) */
        if (args[0].type != dWINDOW || args[1].type != dINTEGER)
            execerror(mc->pc->lineno, "incorrectly typed arguments to winElement()", NULL);
        d = winElement(mc->pc->lineno, args[0].value.win_v, args[1].value.int_v);
        d.flags = 0;
        break;
    }
    default: {		/* unknown function - should not get here */
        execerror(mc->pc->lineno, name.value.str_v, "unknown function");
        break;
    }
    }
    push(mc->stack, d);
    mc->pc = mc->pc + 2;
}

void dumpMap(DataStackEntry *d) {
    GAPLMap *m;
    if (d->type != dMAP) {
        fprintf(stderr, "dumpMap() asked to dump a nonMap");
        return;
    }
    m = d->value.map_v;
    fprintf(stderr, "map elements have type %d\n", m->type);
    fprintf(stderr, "keys: ");
    //ht_dump(m->ht, stderr);
    //need to create an interator, then process the iterator
}
