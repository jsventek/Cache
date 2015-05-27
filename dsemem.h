/* dsemem.h
 *
 * routines for managing a free list of DataStackEntry structs
 *
 */
#ifndef _DSEMEM_H_
#define _DSEMEM_H_

#include "dataStackEntry.h"

DataStackEntry *dse_alloc(void);
void           dse_free(DataStackEntry *dse);

#endif /* _DSEMEM_H_ */
