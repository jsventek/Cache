#ifndef _AUTOMATON_H_
#define _AUTOMATON_H_

/*
 * type and function definitions for an automaton
 */

#include "event.h"
#include "srpc/srpc.h"

typedef struct automaton Automaton;

void          au_init(void);
Automaton     *au_create(char *program, RpcConnection rpc, char *ebuf);
int           au_destroy(unsigned long id);
void          au_publish(unsigned long id, Event *event);
unsigned long au_id(Automaton *au);
Automaton     *au_au(unsigned long id);
RpcConnection au_rpc(Automaton *au);

#endif /* _AUTOMATON_H_ */
