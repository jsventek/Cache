#include "dsemem.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define INCREMENT 4096	/* number added to free list when depleted */

/*
 * private structures used to represent a linked list and list entries
 */

static DataStackEntry *freeList = NULL;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/* assumes that mutex is held */
static void addToFreeList(DataStackEntry *p) {
    p->value.next = freeList;
    freeList = p;
}

void dse_free(DataStackEntry *p) {
    pthread_mutex_lock(&mutex);
    addToFreeList(p);
    pthread_mutex_unlock(&mutex);
}

DataStackEntry *dse_alloc(void) {
    DataStackEntry *p;

    pthread_mutex_lock(&mutex);
    if (!(p = freeList)) {
        p = (DataStackEntry *)malloc(INCREMENT * sizeof(DataStackEntry));
        if (p) {
            int i;

            for (i = 0; i < INCREMENT; i++, p++)
                addToFreeList(p);
            p = freeList;
        }
    }
    if (p)
        freeList = p->value.next;
    pthread_mutex_unlock(&mutex);
    if (p)
        (void)memset(p, 0, sizeof(DataStackEntry));
    return p;
}
