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

/* typetable.c
 *
 * Typetable of supported types
 */

#include "typetable.h"
#include <string.h>

int primtype_val[NUM_PRIMTYPES] = {0,1,2,3,4,5,6,7,8};

const int primtype_size[NUM_PRIMTYPES] = {
    sizeof(char),
    sizeof(int),
    sizeof(double),
    sizeof(char),
    VARIABLESIZE,
    VARIABLESIZE,
    sizeof(char),
    sizeof(short),
    sizeof(long long)
};

const char *primtype_name[NUM_PRIMTYPES] = {
    "boolean",
    "integer",
    "real",
    "character",
    "varchar",
    "blob",
    "tinyint",
    "smallint",
    "timestamp"
};

int typetable_index(char *name) {
    int i;

    for (i = 0; i < NUM_PRIMTYPES; i++)
        if (strcmp(name, primtype_name[i]) == 0)
            return i;
    return -1;
}
