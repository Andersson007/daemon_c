#ifndef TIMEC_H
#define TIMEC_H

#define TS_BUFSIZE 24

void msleep(unsigned long msec);

char* get_now_ts_pretty(time_t ts_epoch);

#endif
