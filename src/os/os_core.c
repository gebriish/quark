#include "os_core.h"

internal OS_Time_Duration
os_time_diff(OS_Time_Stamp start, OS_Time_Stamp end)
{
	OS_Time_Duration result;
	u64 freq = os_time_frequency();
	u64 elapsed = end - start;

	result.seconds = (f64)elapsed / (f64)freq;
	result.milliseconds = result.seconds * 1000.0;
	result.microseconds = result.seconds * 1000000.0;

	return result;
}
