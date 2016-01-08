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

#ifndef _A_GLOBALS_H_
#define _A_GLOBALS_H_

/*
 * externs and struct definitions shared between the parser, the
 * stack machine and the automaton class
 */

#include "adts/tshashmap.h"
#include "adts/hashmap.h"
#include "adts/arraylist.h"
#include "machineContext.h"
#include <pthread.h>

#define MAX_ARGS 20

struct fpargs {
    unsigned int min;	/* minimum number of arguments for builtin */
    unsigned int max;   /* maximum number of arguments for builtin */
    unsigned int index;	/* index used for switch statements */
};

/* declared in agram.y */
extern ArrayList *variables;
extern ArrayList *index2vars;
extern HashMap *topics;
extern HashMap *sourcefilters;
extern HashMap *builtins;
extern char *progname;
/* declared in code.c */
extern InstructionEntry *progp, *startp, *initialization, *behavior;
extern int initSize, behavSize;
extern int iflog;
/* declared in automaton.c */
extern pthread_key_t jmpbuf_key;
extern pthread_key_t execerr_key;

#endif /* _A_GLOBALS_H_ */
