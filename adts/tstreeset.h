#ifndef _TSTREESET_H_
#define _TSTREESET_H_

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

#include "tsiterator.h"

/*
 * interface definition for thread-safe generic treeset implementation
 *
 * patterned roughly after Java 6 TreeSet generic class
 */

typedef struct tstreeset TSTreeSet;	/* opaque type definition */

/*
 * create a treeset that is ordered using `cmpFunction' to compare two elements
 *
 * returns a pointer to the treeset, or NULL if there are malloc() errors
 */
TSTreeSet *tsts_create(int (*cmpFunction)(void *, void *));

/*
 * destroys the treeset; for each element, if userFunction != NULL,
 * it is invoked on that element; the storage associated with
 * the treeset is then returned to the heap
 */
void tsts_destroy(TSTreeSet *ts, void (*userFunction)(void *));

/*
 * obtains the lock for exclusive access
 */
void tsts_lock(TSTreeSet *ts);

/*
 * returns the lock
 */
void tsts_unlock(TSTreeSet *ts);

/*
 * adds the specified element to the set if it is not already present
 *
 * returns 1 if the element was added, 0 if the element was already present
 */
int tsts_add(TSTreeSet *ts, void *element);

/*
 * returns the least element in the set greater than or equal to `element'
 *
 * returns 1 if found, or 0 if no such element
 */
int tsts_ceiling(TSTreeSet *ts, void *element, void **ceiling);

/*
 * clears all elements from the treeset; for each element,
 * if userFunction != NULL, it is invoked on that element;
 * any storage associated with that element in the treeset is then
 * returned to the heap
 *
 * upon return, the treeset will be empty
 */
void tsts_clear(TSTreeSet *ts, void (*userFunction)(void *));

/*
 * returns 1 if the set contains the specified element, 0 if not
 */
int tsts_contains(TSTreeSet *ts, void *element);

/*
 * returns the first (lowest) element currently in the set
 *
 * returns 1 if non-empty, 0 if empty
 */
int tsts_first(TSTreeSet *ts, void **element);

/*
 * returns the greatest element in the set less than or equal to `element'
 *
 * returns 1 if found, or 0 if no such element
 */
int tsts_floor(TSTreeSet *ts, void *element, void **floor);

/*
 * returns the least element in the set strictly greater than `element'
 *
 * returns 1 if found, or 0 if no such element
 */
int tsts_higher(TSTreeSet *ts, void *element, void **higher);

/*
 * returns 1 if set is empty, 0 if it is not
 */
int tsts_isEmpty(TSTreeSet *ts);

/*
 * returns the last (highest) element currently in the set
 *
 * returns 1 if non-empty, 0 if empty
 */
int tsts_last(TSTreeSet *ts, void **element);

/*
 * returns the greatest element in the set strictly less than `element'
 *
 * returns 1 if found, or 0 if no such element
 */
int tsts_lower(TSTreeSet *ts, void *element, void **lower);

/*
 * retrieves and removes the first (lowest) element
 *
 * returns 0 if set was empty, 1 otherwise
 */
int tsts_pollFirst(TSTreeSet *ts, void **element);

/*
 * retrieves and removes the last (highest) element
 *
 * returns 0 if set was empty, 1 otherwise
 */
int tsts_pollLast(TSTreeSet *ts, void **element);

/*
 * removes the specified element from the set if present
 * if userFunction != NULL, invokes it on the element before removing it
 *
 * returns 1 if successful, 0 if not present
 */
int tsts_remove(TSTreeSet *ts, void *element, void (*userFunction)(void *));

/*
 * returns the number of elements in the treeset
 */
long tsts_size(TSTreeSet *ts);

/*
 * return the elements of the treeset as an array of void * pointers
 * the order of elements in the array is the as determined by the treeset's
 * compare function
 *
 * returns pointer to the array or NULL if error
 * returns number of elements in the array in len
 */
void **tsts_toArray(TSTreeSet *ts, long *len);

/*
 * create generic iterator to this treeset
 *
 * returns pointer to the Iterator or NULL if failure
 */
TSIterator *tsts_it_create(TSTreeSet *ts);

#endif /* _TSTREESET_H_ */
