
#ifndef _SRPCMALLOC_H_
#define _SRPCMALLOC_H_
#include <stdlib.h>

void *srpc_malloc(size_t size);

void *srpc_calloc(size_t nmemb, size_t size);

void srpc_free(void *ptr);

void srpc_dump(void);

#endif /* _SRPCMALLOC_H_ */
