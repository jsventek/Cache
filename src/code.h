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

#ifndef _CODE_H_
#define _CODE_H_

/*
 * externs and instructions shared by the parser and the stack machine
 * interpreter
 */

#include "machineContext.h"
#include <stdio.h>

void initcode(void);
void switchcode(void);
void endcode(void);
void initstack(void);
void execute(MachineContext *mc, InstructionEntry *i);
void constpush(MachineContext *mc);
void varpush(MachineContext *mc);
void add(MachineContext *mc);
void subtract(MachineContext *mc);
void multiply(MachineContext *mc);
void divide(MachineContext *mc);
void modulo(MachineContext *mc);
void negate(MachineContext *mc);
void eval(MachineContext *mc);
void extract(MachineContext *mc);
void gt(MachineContext *mc);
void ge(MachineContext *mc);
void lt(MachineContext *mc);
void le(MachineContext *mc);
void eq(MachineContext *mc);
void ne(MachineContext *mc);
void and(MachineContext *mc);
void or(MachineContext *mc);
void not(MachineContext *mc);
void function(MachineContext *mc);
void print(MachineContext *mc);
void assign(MachineContext *mc);
void pluseq(MachineContext *mc);
void minuseq(MachineContext *mc);
void whilecode(MachineContext *mc);
void ifcode(MachineContext *mc);
void procedure(MachineContext *mc);
void newmap(MachineContext *mc);
void newwindow(MachineContext *mc);
void destroy(MachineContext *mc);
void bitOr(MachineContext *mc);
void bitAnd(MachineContext *mc);
InstructionEntry *code(int ifInst, Inst f, DataStackEntry *d, char *what,
                       int lineno);
DataStackEntry *dse_duplicate(DataStackEntry d);
GAPLMap *map_duplicate(GAPLMap m);
GAPLWindow *win_duplicate(GAPLWindow w);
GAPLWindowEntry *we_duplicate(GAPLWindowEntry w);
void warning(char *b, int lineno, char *s1, char *s2);
void execerror(int lineno, char *s1, char *s2);
void comperror(char *s1, char *s2);
void dumpMap(DataStackEntry *d);
void dumpProgramBlock(InstructionEntry *i, int size);
void dumpDataStackEntry (DataStackEntry *d, int verbose);
void printDSE(DataStackEntry *d, FILE *fd);
void freeDSE(DataStackEntry *d);
void dumpCompilationResults(unsigned long id,
                            ArrayList *v, ArrayList *i2v,
                            InstructionEntry *init, int initSize,
                            InstructionEntry *behav, int behavSize);

extern InstructionEntry *initialization, *behavior;

#endif /* _CODE_H_ */
