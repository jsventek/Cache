/*
 * tuple.h - define data structure for tuples
 */
#ifndef _TUPLE_H_
#define _TUPLE_H_

#define MAX_TUPLE_SIZE 4096

union Tuple {
    unsigned char bytes[MAX_TUPLE_SIZE];
    char *ptrs[MAX_TUPLE_SIZE/sizeof(char *)];
};

#endif /* _TUPLE_H_ */
