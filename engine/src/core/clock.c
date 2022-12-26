#include "clock.h"
#include "platform/platform.h"

void clock_update(clock* clock) {
    if (clock->start_time == 0) return;
    clock->elapsed = platform_get_absolute_time() - clock->start_time;
}

void clock_start(clock* clock) {
    clock->start_time = platform_get_absolute_time();
}

void clock_stop(clock* clock){
    clock->start_time = 0;
}
