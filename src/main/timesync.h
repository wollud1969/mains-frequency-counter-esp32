#ifndef _TIMESYNC_H_
#define _TIMESYNC_H_

#include <stdbool.h>


#define SNTP_SERVER "pool.ntp.org"


void timesyncInit();
bool timesyncReady();

#endif // _TIMESYNC_H_



