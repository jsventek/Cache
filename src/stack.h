#ifndef _STACK_H_
#define _STACK_H_
/*
 * data stack for use with automaton infrastructure
 */

#include "dataStackEntry.h"

typedef struct stack Stack;

Stack          *stack_create(int size);
void           stack_destroy(Stack *st);
void           reset(Stack *st);
void           push(Stack *st, DataStackEntry d);
DataStackEntry pop(Stack *st);

#endif /* _STACK_H_ */
