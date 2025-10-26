#ifndef TIMECLOCKRANDOM_H
#define TIMECLOCKRANDOM_H

#include <stdlib.h>
#include <time.h>

inline void ClockRandomInit() {
	srand((unsigned)time(NULL));
}

inline double getClockRandom() {
	return rand() / (RAND_MAX + 1.0);
}

#endif