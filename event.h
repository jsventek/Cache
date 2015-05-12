#ifndef _EVENT_H_
#define _EVENT_H_

#include "dataStackEntry.h"

typedef struct event Event;

Event *ev_create(char *name, char *eventData, unsigned long nAUs);
Event *ev_reference(Event *event);
void  ev_release(Event *event);
char  *ev_data(Event *event);
char  *ev_topic(Event *event);
int   ev_theData(Event *event, DataStackEntry **dse);
void  ev_dump(Event *event);

#endif /* _EVENT_H_ */
