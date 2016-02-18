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
 * data stack for use with automaton infrastructure
 */

#include "stack.h"
#include "code.h"
#include <stdlib.h>

#define DEFAULT_STACK_SIZE 256

struct stack {
    int size;
    DataStackEntry *theStack;
    DataStackEntry *sp;
    DataStackEntry *limit;
};

Stack *stack_create(int size) {
    int N = (size > 0) ? size : DEFAULT_STACK_SIZE;
    Stack *st = (Stack *)malloc(sizeof(Stack));
    if (st) {
        st->theStack = (DataStackEntry *)malloc(N*sizeof(DataStackEntry));
        if (! st->theStack) {
            free(st);
            st = NULL;
        } else {
            st->size = N;
            st->sp = st->theStack;
            st->limit = st->sp + N;
        }
    }
    return st;
}

void stack_destroy(Stack *st) {
    free(st->theStack);
    free(st);
}

void reset(Stack *st) {
    st->sp = st->theStack;
}

void push(Stack *st, DataStackEntry d) {
    if (st->sp >= st->limit)
        execerror(999, "stack overflow", NULL);
    *(st->sp++) = d;
}

DataStackEntry pop(Stack *st) {
    if (st->sp <= st->theStack)
        execerror(999, "stack underflow", NULL);
    return *(--st->sp);
}
