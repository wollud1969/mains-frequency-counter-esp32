#ifndef _TIMESYNC_H_
#define _TIMESYNC_H_

#include <stdbool.h>
#include <stdint.h>

#define SNTP_SERVER "pool.ntp.org"


void timesyncInit();
bool timesyncReady();
uint32_t timesyncGetCurrentSeconds();

#endif // _TIMESYNC_H_



