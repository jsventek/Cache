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

/**
 * Timestamp
 *
 */
#include "timestamp.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int daylight;
extern long timezone;

tstamp_t current_time = 0ULL;

tstamp_t timestamp_now(void) {
    struct timeval tv;

    if (current_time)
        return current_time;
    gettimeofday(&tv, NULL);
    return timeval_to_timestamp(&tv);
}

char *timestamp_to_string(tstamp_t ts) {
    char b[64], *s;
    sprintf(b, "@%016llx@", ts & ~DROPPED);	/* ignore dropped bit if set */
    s = strdup(b);
    return s;
}

tstamp_t string_to_timestamp(char *s) {
    tstamp_t ts;

    ts = strtoull(s+1, NULL, 16);	/* skip over leading @ */
    return ts;
}

tstamp_t timeval_to_timestamp(struct timeval *tv) {
    tstamp_t ts;

    ts = 1000 * (1000000 * (tstamp_t)(tv->tv_sec) + (tstamp_t)(tv->tv_usec));
    return ts;
}

void timestamp_to_timeval(tstamp_t ts, struct timeval *tv) {
    tv->tv_usec = (unsigned long)(ts % 1000000000LL) / 1000;
    tv->tv_sec = (unsigned long) (ts / 1000000000LL);
}

tstamp_t timestamp_add_incr(tstamp_t ts, unsigned long units, int ifmillis) {
    tstamp_t ans;
    unsigned long long mult;

    mult = (ifmillis) ? 1000000LL : 1000000000LL;
    ans = ts + mult * (unsigned long long)units;
    return ans;
}

tstamp_t timestamp_sub_incr(tstamp_t ts, unsigned long units, int ifmillis) {
    tstamp_t ans;
    unsigned long long mult;

    mult = (ifmillis) ? 1000000LL : 1000000000LL;
    ans = ts - mult * (unsigned long long)units;
    return ans;
}

/*
 * converts date string to a timestamp
 *
 * datestring is of the form: YYYY/MM/DD:hh:mm:ss
 *
 * YYYY must be >= 1970; if not, it is set to 1970
 * MM must be >= 1 and <= 12; if not, it is set to 1
 * DD must be >= 1 and <= 31; if not, it is set to 1
 * hh must be >= 0 and <= 23; if not, it is set to 0
 * mm must be >= 0 and <= 59; if not, it is set to 0
 * ss must be >= 0 and <= 61; if not, it is set to 0
 * (note, ss = 60 or ss = 61 are when accounting for leap seconds)
 */

tstamp_t datestring_to_timestamp(char *s) {
    struct timespec ts;
    struct tm tmv;
    int year, month, day, hh, mm, ss;
    int save_daylight = daylight;
    long save_timezone = timezone;

    sscanf(s, "%d/%d/%d:%d:%d:%d", &year, &month, &day, &hh, &mm, &ss);
    tmv.tm_year = (year >= 1970) ? year - 1900 : 70;
    tmv.tm_mon = (month > 0 && month < 13) ? month - 1 : 0;
    tmv.tm_mday = (day > 0 && day < 32) ? day : 1;
    tmv.tm_hour = (hh >= 0 && hh < 24) ? hh : 0;
    tmv.tm_min = (mm >= 0 && mm < 60) ? mm : 0;
    tmv.tm_sec = (ss >= 0 && ss < 62) ? ss : 0;
    tmv.tm_isdst = 0;
    daylight = 0;
    timezone = 0;
    ts.tv_sec = mktime(&tmv);
    ts.tv_nsec = 0;
    daylight = save_daylight;
    timezone = save_timezone;
    return timeval_to_timestamp((struct timeval *)&ts);
}

char *timestamp_to_datestring(tstamp_t ts) {
    char buf[64];
    struct timeval tv;
    struct tm tmv;
    int save_daylight;
    long save_timezone;

    timestamp_to_timeval(ts, &tv);
    save_daylight = daylight;
    daylight = 0;
    save_timezone = timezone;
    timezone = 0;
    (void) gmtime_r(&tv.tv_sec, &tmv);
    daylight = save_daylight;
    timezone = save_timezone;
    sprintf(buf, "%4d/%02d/%02d:%02d:%02d:%02d", tmv.tm_year+1900, tmv.tm_mon+1,
            tmv.tm_mday, tmv.tm_hour, tmv.tm_min, tmv.tm_sec);
    return strdup(buf);
}

/*
 * Similar to the timestamp_to_datestring() function above.
 *
 * Returns a string representation of the current date/time,
 * suitable for DB filenames: YYYYMMDDHHMMSS.
 *
 * As an example, for a database Foo the filename is:
 *
 * 	FooYYYYMMDDHHMMSS.db
 */
char *timestamp_to_filestring(tstamp_t ts) {
    char buf[64];
    struct timeval tv;
    struct tm tmv;
    int save_daylight;
    long save_timezone;

    timestamp_to_timeval(ts, &tv);
    save_daylight = daylight;
    daylight = 0;
    save_timezone = timezone;
    timezone = 0;
    (void) gmtime_r(&tv.tv_sec, &tmv);
    daylight = save_daylight;
    timezone = save_timezone;
    sprintf(buf, "%4d%02d%02d%02d%02d%02d", tmv.tm_year+1900, tmv.tm_mon+1,
            tmv.tm_mday, tmv.tm_hour, tmv.tm_min, tmv.tm_sec);
    return strdup(buf);
}

/*
 * Function used when rotating persistent DB files.
 *
 * Returns the number of seconds from now until 23:59:59.
 *
 * Then adds 2 seconds (which results in the time difference
 * from now until 00:00:01).
 */

int seconds_to_midnight(tstamp_t ts) {
    struct timeval tv;
    struct tm tmv;
    int save_daylight;
    long save_timezone;
    timestamp_to_timeval(ts, &tv);
    save_daylight = daylight;
    daylight = 0;
    save_timezone = timezone;
    timezone = 0;
    (void) gmtime_r(&tv.tv_sec, &tmv);
    daylight = save_daylight;
    timezone = save_timezone;
    return (60*(60*(23-tmv.tm_hour) + (59-tmv.tm_min)) + (59-tmv.tm_sec) + 2);
}
