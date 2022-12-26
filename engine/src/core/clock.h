#pragma once
#include "defines.h"

typedef struct clock {
    f64 start_time;
    f64 elapsed;
} clock;

//should be called before checking elpased time.
//no effect on non started clocks.
void clock_update(clock* clock);

//start given clock. Resets elpased time.
void clock_start(clock* clock);

//stop given clock. Does resets elpased time.
void clock_stop(clock* clock);
