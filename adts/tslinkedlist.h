#ifndef _TSLINKEDLIST_H_
#define _TSLINKEDLIST_H_

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

/* interface definition for thread-safe generic linked list
 *
 * patterned roughly after Java 6 LinkedList generic class, with many
 * duplicate methods removed
 */

#include "tsiterator.h"

typedef struct tslinkedlist TSLinkedList;

/*
 * create a linked list
 *
 * returns a pointer to the linked list, or NULL if there are malloc() errors
 */
TSLinkedList *tsll_create(void);

/*
 * destroys the linked list; for each element, if userFunction != NULL, invokes
 * userFunction on the element and then returns any list structures to the heap
 * then completely deletes the list structures
 */
void tsll_destroy(TSLinkedList *ll, void (*userFunction)(void *element));

/*
 * obtains the lock for exclusive access
 */
void tsll_lock(TSLinkedList *ll);

/*
 * returns the lock
 */
void tsll_unlock(TSLinkedList *ll);

/*
 * appends `element' to the end of the list
 *
 * returns 1 if successful, 0 if unsuccesful (malloc errors)
 */
int tsll_add(TSLinkedList *ll, void *element);

/*
 * inserts `element' at the specified position in the list;
 * all elements from `index' upwards are shifted one position;
 * if current size is N, 0 <= index <= N must be true
 *
 * returns 1 if successful, 0 if unsuccessful (malloc errors)
 */
int tsll_insert(TSLinkedList *ll, long i, void *element);

/*
 * inserts `element' at the beginning of the list
 * equivalent to tsll_insert(ll, 0, element);
 */
int tsll_addFirst(TSLinkedList *ll, void *element);

/*
 * appends `element' at the end of the list
 * equivalent to tsll_add(ll, element);
 */
int tsll_addLast(TSLinkedList *ll, void *element);

/*
 * clears the linked list; for each element, if userFunction != NULL, invokes
 * userFunction on the element and then returns any list structures to the heap
 * upon return, the list is empty
 */
void tsll_clear(TSLinkedList *ll, void (*userFunction)(void *element));

/*
 * Retrieves, but does not remove, the element at the specified index
 *
 * return 1 if successful, 0 if not
 */
int tsll_get(TSLinkedList *ll, long index, void **element);

/*
 * Retrieves, but does not remove, the first element
 *
 * return 1 if successful, 0 if not
 */
int tsll_getFirst(TSLinkedList *ll, void **element);

/*
 * Retrieves, but does not remove, the last element
 *
 * return 1 if successful, 0 if not
 */
int tsll_getLast(TSLinkedList *ll, void **element);

/*
 * Retrieves, and removes, the element at the specified index
 *
 * return 1 if successful, 0 if not
 */
int tsll_remove(TSLinkedList *ll, long index, void **element);

/*
 * Retrieves, and removes, the first element
 *
 * return 1 if successful, 0 if not
 */
int tsll_removeFirst(TSLinkedList *ll, void **element);

/*
 * Retrieves, and removes, the last element
 *
 * return 1 if successful, 0 if not
 */
int tsll_removeLast(TSLinkedList *ll, void **element);

/*
 * Replaces the element at the specified index; the previous element
 * is returned in `*previous'
 *
 * return 1 if successful, 0 if not
 */
int tsll_set(TSLinkedList *ll, long index, void *element, void **previous);

/*
 * returns the number of elements in the linked list
 */
long tsll_size(TSLinkedList *ll);

/*
 * returns an array containing all of the elements of the linked list in
 * proper sequence (from first to last element); returns the length of the
 * list in `len'
 *
 * returns poijnter to void * array of elements, or NULL if malloc failure
 */
void **tsll_toArray(TSLinkedList *ll, long *len);

/*
 * creates an iterator for running through the linked list
 *
 * returns pointer to the Iterator or NULL
 */
TSIterator *tsll_it_create(TSLinkedList *ll);

#endif /* _TSLINKEDLIST_H_ */
