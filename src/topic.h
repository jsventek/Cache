#ifndef _TOPIC_H_
#define _TOPIC_H_

#include "automaton.h"

typedef struct schemaCell {
    char *name;
    int type;
} SchemaCell;

void top_init(void);
int  top_exist(char *name);
int  top_create(char *name, char *schema);
int  top_publish(char *name, char *message);
int  top_subscribe(char *name, unsigned long id);
void top_unsubscribe(char *name, unsigned long id);
int  top_schema(char *name, int *ncells, SchemaCell **schema);
int  top_index(char *name, char *colname);

#endif /* _TOPIC_H_ */
