#ifndef _TSARRAYLIST_H_
#define _TSARRAYLIST_H_

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
 * interface definition for thread-safe generic arraylist implementation
 *
 * patterned roughly after Java 6 ArrayList generic class
 */

typedef struct tsarraylist TSArrayList;	/* opaque type definition */

/*
 * create an arraylist with the specified capacity; if capacity == 0, a
 * default initial capacity (10 elements) is used
 *
 * returns a pointer to the array list, or NULL if there are malloc() errors
 */
TSArrayList *tsal_create(long capacity);

/*
 * destroys the arraylist; for each occupied index, if userFunction != NULL,
 * it is invoked on the element at that position; the storage associated with
 * the arraylist is then returned to the heap
 */
void tsal_destroy(TSArrayList *al, void (*userFunction)(void *element));

/*
 * obtains the lock for exclusive access
 */
void tsal_lock(TSArrayList *al);

/*
 * returns the lock
 */
void tsal_unlock(TSArrayList *al);

/*
 * appends `element' to the arraylist; if no more room in the list, it is
 * dynamically resized
 *
 * returns 1 if successful, 0 if unsuccessful (malloc errors)
 */
int tsal_add(TSArrayList *al, void *element);

/*
 * clears all elements from the arraylist; for each occupied index,
 * if userFunction != NULL, it is invoked on the element at that position;
 * any storage associated with the element in the arraylist is then
 * returned to the heap
 *
 * upon return, the arraylist will be empty
 */
void tsal_clear(TSArrayList *al, void (*userFunction)(void *element));

/*
 * ensures that the arraylist can hold at least `minCapacity' elements
 *
 * returns 1 if successful, 0 if unsuccessful (malloc failure)
 */
int tsal_ensureCapacity(TSArrayList *al, long minCapacity);

/*
 * returns the element at the specified position in this list in `*element'
 *
 * returns 1 if successful, 0 if no element at that position
 */
int tsal_get(TSArrayList *al, long i, void **element);

/*
 * inserts `element' at the specified position in the arraylist;
 * all elements from `i' onwards are shifted one position to the right;
 * if no more room in the list, it is dynamically resized;
 * if the current size of the list is N;
 * legal values of i are in the interval [0, N]
 *
 * returns 1 if successful, 0 if unsuccessful (malloc errors)
 */
int tsal_insert(TSArrayList *al, long i, void *element);

/*
 * returns 1 if list is empty, 0 if it is not
 */
int tsal_isEmpty(TSArrayList *al);

/*
 * removes the `i'th element from the list, returns the value that
 * occupied that position in `*element'
 *
 * returns 1 if successful, 0 if `i'th position was not occupied
 */
int tsal_remove(TSArrayList *al, long i, void **element);

/*
 * relaces the `i'th element of the arraylist with `element';
 * returns the value that previously occupied that position in `previous'
 *
 * returns 1 if successful
 * returns 0 if `i'th position not currently occupied
 */
int tsal_set(TSArrayList *al, void *element, long i, void **previous);

/*
 * returns the number of elements in the arraylist
 */
long tsal_size(TSArrayList *al);

/*
 * returns an array containing all of the elements of the list in
 * proper sequence (from first to last element); returns the length of
 * the list in `len'
 *
 * returns pointer to void * array of elements, or NULL if malloc failure
 */
void **tsal_toArray(TSArrayList *al, long *len);

/*
 * trims the capacity of the arraylist to be the list's current size
 *
 * returns 1 if successful, 0 if failure (malloc errors)
 */
int tsal_trimToSize(TSArrayList *al);

/*
 * create generic iterator to this arraylist
 *
 * returns pointer to the Iterator or NULL if failure
 */
TSIterator *tsal_it_create(TSArrayList *al);

#endif /* _TSARRAYLIST_H_ */
