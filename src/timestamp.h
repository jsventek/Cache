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
 * timestamp.h - data structures and methods for timestamps
 */
#ifndef _TIMESTAMP_H_
#define _TIMESTAMP_H_

#include <sys/time.h>

#define DROPPED 0x8000000000000000LL	/* bit that is manipulated in tstamp
                                           when packet is to be dropped */

typedef unsigned long long tstamp_t;

extern tstamp_t current_time;

tstamp_t timestamp_now(void);
char     *timestamp_to_string(tstamp_t ts);
tstamp_t string_to_timestamp(char *s);
void     timestamp_to_timeval(tstamp_t ts, struct timeval *tv);
tstamp_t timeval_to_timestamp(struct timeval *tv);
tstamp_t timestamp_add_incr(tstamp_t ts, unsigned long units, int ifmillis);
tstamp_t timestamp_sub_incr(tstamp_t ts, unsigned long units, int ifmillis);
tstamp_t datestring_to_timestamp(char *s);
char     *timestamp_to_datestring(tstamp_t ts);
char     *timestamp_to_filestring(tstamp_t ts);
int      seconds_to_midnight(tstamp_t ts);

#endif /* _TIMESTAMP_H_ */
