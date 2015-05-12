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

#ifndef _MACHINECONTEXT_H_
#define _MACHINECONTEXT_H_

/*
 * execution context for automaton virtual machine
 */

#include "dataStackEntry.h"
#include "hashmap.h"
#include "arraylist.h"
#include "stack.h"
#include "automaton.h"

typedef struct instructionEntry InstructionEntry;

typedef struct machineContext {
    ArrayList *variables;
    ArrayList *index2vars;
    Stack *stack;
    char *currentTopic;
    Automaton *au;
    InstructionEntry *pc;
} MachineContext;

/*
 * instructions for the stack machine are instances of the following union
 */

typedef void (*Inst)(MachineContext *mc);  /* actual machine instruction */
#define STOP (Inst)0

#define FUNC 1
#define DATA 2
#define PNTR 3

struct instructionEntry {
    int type;
    union {
        Inst op;
        DataStackEntry immediate;
        int offset;			/* offset from current PC */
    } u;
    char *label;
    int lineno;				/* corresponding source line no */
};

#endif /* _MACHINECONTEXT_H_ */
