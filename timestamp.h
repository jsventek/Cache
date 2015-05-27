/*
 * timestamp.h - data structures and methods for timestamps
 */
#ifndef _TIMESTAMP_H_
#define _TIMESTAMP_H_

#include <sys/time.h>

#define DROPPED 0x8000000000000000LL	/* bit that is manipulated in tstamp
                                           when packet is to be dropped */

typedef unsigned long long tstamp_t;

extern tstamp_t current_time;

tstamp_t timestamp_now(void);
char     *timestamp_to_string(tstamp_t ts);
tstamp_t string_to_timestamp(char *s);
void     timestamp_to_timeval(tstamp_t ts, struct timeval *tv);
tstamp_t timeval_to_timestamp(struct timeval *tv);
tstamp_t timestamp_add_incr(tstamp_t ts, unsigned long units, int ifmillis);
tstamp_t timestamp_sub_incr(tstamp_t ts, unsigned long units, int ifmillis);
tstamp_t datestring_to_timestamp(char *s);
char     *timestamp_to_datestring(tstamp_t ts);
char     *timestamp_to_filestring(tstamp_t ts);
int      seconds_to_midnight(tstamp_t ts);

#endif /* _TIMESTAMP_H_ */
