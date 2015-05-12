#ifndef _TSHASHMAP_H_
#define _TSHASHMAP_H_

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
#include "hashmap.h"

/*
 * interface definition for thread-safe generic hashmap implementation
 *
 * patterned roughly after Java 6 HashMap generic class with String keys
 */

typedef struct tshashmap TSHashMap;	/* opaque type definition */

/*
 * create a hashmap with the specified capacity and load factor;
 * if capacity == 0, a default initial capacity (16 elements) is used
 * if loadFactor == 0.0, a default load factor (0.75) is used
 * if number of elements/number of buckets ever exceeds the load factor,
 * the hashmap is resized by doubling the number of buckets, up to
 * a maximum number of buckets (134,217,728)
 *
 * returns a pointer to the hashmap, or NULL if there are malloc() errors
 */
TSHashMap *tshm_create(long capacity, double loadFactor);

/*
 * destroys the hashmap; for each HMEntry, if userFunction != NULL,
 * it is invoked on the element in that entry ; the storage associated with
 * the hashmap is then returned to the heap
 */
void tshm_destroy(TSHashMap *hm, void (*userFunction)(void *element));

/*
 * obtains the lock for exclusive access
 */
void tshm_lock(TSHashMap *hm);

/*
 * returns the lock
 */
void tshm_unlock(TSHashMap *hm);

/*
 * clears all elements from the hashmap; for each HMEntry,
 * if userFunction != NULL, it is invoked on the element in that entry;
 * any storage associated with the entry in the hashmap is then
 * returned to the heap
 *
 * upon return, the hashmap will be empty
 */
void tshm_clear(TSHashMap *hm, void (*userFunction)(void *element));

/*
 * returns 1 if hashmap has an entry for `key', 0 otherwise
 */
int tshm_containsKey(TSHashMap *hm, char *key);

/*
 * returns an array containing all of the entries of the hashmap in
 * an arbitrary order; returns the length of the list in `len'
 *
 * returns pointer to HMEntry * array of elements, or NULL if malloc failure
 */
HMEntry **tshm_entryArray(TSHashMap *hm, long *len);

/*
 * returns the element to which the specified key is mapped in `*element'
 *
 * returns 1 if successful, 0 if no mapping for `key'
 */
int tshm_get(TSHashMap *hm, char *key, void **element);

/*
 * returns 1 if hashmap is empty, 0 if it is not
 */
int tshm_isEmpty(TSHashMap *hm);

/*
 * returns an array containing all of the keys in the hashmap in
 * an arbitrary order; returns the length of the list in `len'
 *
 * returns pointer to char * array of keys, or NULL if malloc failure
 */
char **tshm_keyArray(TSHashMap *hm, long *len);

/*
 * associates `element' with key'; if this replaces an existing mapping, the
 * old value is returned in `*previous'
 *
 *
 * returns 1 if successful, 0 if not (malloc failure)
 */
int tshm_put(TSHashMap *hm, char *key, void *element, void **previous);

/*
 * removes the entry associated with `key' if one exists; returns element
 * associated with key in `*element'
 *
 * returns 1 if successful, 0 if `i'th position was not occupied
 */
int tshm_remove(TSHashMap *hm, char *key, void **element);

/*
 * returns the number of mappings in the hashmap
 */
long tshm_size(TSHashMap *hm);

/*
 * create generic iterator to this arraylist
 * note that iterator will return pointers to HMEntry's
 *
 * returns pointer to the Iterator or NULL if failure
 */
TSIterator *tshm_it_create(TSHashMap *hm);

#endif /* _TSHASHMAP_H_ */
