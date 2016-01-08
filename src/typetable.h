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

#ifndef _TYPETABLE_H_
#define _TYPETABLE_H_

#define NUM_PRIMTYPES 9
#define VARIABLESIZE -1

extern int primtype_val[];

#define PRIMTYPE_BOOLEAN   &primtype_val[0]
#define PRIMTYPE_INTEGER   &primtype_val[1]
#define PRIMTYPE_REAL      &primtype_val[2]
#define PRIMTYPE_CHARACTER &primtype_val[3]

#define PRIMTYPE_VARCHAR   &primtype_val[4]
#define PRIMTYPE_BLOB      &primtype_val[5]

#define PRIMTYPE_TINYINT   &primtype_val[6]
#define PRIMTYPE_SMALLINT  &primtype_val[7]

#define PRIMTYPE_TIMESTAMP &primtype_val[8]

extern const int primtype_size[];

extern const char *primtype_name[];

/*
 * looks up the string type name and returns the corresponding array index
 * returns -1 if not found
 */
int typetable_index(char *name);

#endif /* _TYPETABLE_H_ */
