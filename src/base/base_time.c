#include "base_time.h"

#if OS_WINDOWS
# include <windows.h>
#elif OS_LINUX || OS_MAC
# include <time.h>
# include <sys/time.h>
#endif


#if OS_WINDOWS // TODO(geb) Needs testing

internal_lnk Time_Stamp
time_now() 
{
  LARGE_INTEGER counter;
  QueryPerformanceCounter(&counter);
  return (Time_Stamp)counter.QuadPart;
}

internal_lnk u64
time_frequency() 
{
  LARGE_INTEGER freq;
  QueryPerformanceFrequency(&freq);
  return (u64)freq.QuadPart;
}

internal_lnk void
time_sleep_ms(u32 milliseconds) 
{
  Sleep(milliseconds);
}

#elif OS_LINUX || OS_MAC

internal_lnk Time_Stamp
time_now() 
{
  struct timespec ts;
#if OS_LINUX
  clock_gettime(CLOCK_MONOTONIC, &ts);
#elif OS_MAC
  clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
#endif
  return (Time_Stamp)ts.tv_sec * 1000000000ULL + (Time_Stamp)ts.tv_nsec;
}

internal_lnk u64 
time_frequency() 
{
  return 1000000000ULL;
}

internal_lnk void 
time_sleep_ms(u32 milliseconds) 
{
  struct timespec ts;
  ts.tv_sec = milliseconds / 1000;
  ts.tv_nsec = (milliseconds % 1000) * 1000000;
  nanosleep(&ts, NULL);
}

#else
# error Platform not supported for timer implementation
#endif

internal_lnk Time_Duration
time_diff(Time_Stamp start, Time_Stamp end) 
{
  Time_Duration result;
  u64 freq = time_frequency();
  u64 elapsed = end - start;
  
  result.seconds = (f64)elapsed / (f64)freq;
  result.milliseconds = result.seconds * 1000.0;
  result.microseconds = result.seconds * 1000000.0;
  
  return result;
}
