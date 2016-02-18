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
 * filecrawler.h
 */
#ifndef HW_FILECRAWLER_H
#define HW_FILECRAWLER_H

#include "sqlstmts.h"
#include "rtab.h"

#include "map.h"

#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>

typedef struct filecrawler {
    Ndx *n;
    struct stat sb;
    int empty;
    int *drop;
} Filecrawler;

Filecrawler *filecrawler_new(char *tablename);

void filecrawler_free(Filecrawler *fc);

void filecrawler_window(Filecrawler *fc, sqlwindow *win);

void filecrawler_filter(Filecrawler *fc, T *t, int nfilters,
                        sqlfilter **filters, int filtertype);

void filecrawler_project (Filecrawler *fc, T *t, Rtab *results);

long long filecrawler_count (Filecrawler *fc); /* count non-dropped */

void filecrawler_reset (Filecrawler *fc); /* clear dropped */

long long filecrawler_dump (Filecrawler *fc);

#endif

