#ifndef __REQUEST_H__
#define __REQUEST_H__

#include "System.h"

void requestHandle(int fd, int thread_id, Stats* stats, struct timeval arrival, struct timeval dispatch);

//I will save shabbas and pass 130 tests

#endif
