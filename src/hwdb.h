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

#ifndef _HWDB_H_
#define _HWDB_H_

#include "rtab.h"
#include "pubsub.h"
#include "srpc/srpc.h"
#include "table.h"
#include "automaton.h"
#include "sqlstmts.h"

#define SUBSCRIPTION 1
#define REGISTRATION 2

typedef struct callBackInfo {
    short type, ifdisconnect;	/* value of SUBSCRIPTION or REGISTRATION */
    char *str;                  /* return value to send, if any */
    union {
        long id;
        Automaton *au;
    } u;
} CallBackInfo;

int hwdb_init(int usesRPC);
Rtab *hwdb_exec_query(char *query, int isreadonly);
int hwdb_send_event(Automaton *au, char *buf, int ifdisconnect);
Table *hwdb_table_lookup(char *name);
void hwdb_queue_cleanup(CallBackInfo *info);
int hwdb_insert(sqlinsert *insert);

#endif /* _HWDB_H_ */
