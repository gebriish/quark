#ifndef BASE_TIME_H
#define BASE_TIME_H

#include "base_core.h"

typedef u64 Time_Stamp;

typedef struct {
  f64 seconds;
  f64 milliseconds;
  f64 microseconds;
} Time_Duration;


////////////////////////////////
// ~geb: OS Specific

internal_lnk Time_Stamp time_now();
internal_lnk u64 time_frequency();
internal_lnk void time_sleep_ms(u32 milliseconds);

////////////////////////////////
// ~geb: Timing API

internal_lnk Time_Duration time_diff(Time_Stamp start, Time_Stamp end);

#endif

