#ifndef OS_CORE_LINUX_H
#define OS_CORE_LINUX_H

#include "../os_core.h"

#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>

internal OS_FileProps os_linx_file_props_from_stats(struct stat *s);
internal u64 os_linx_time_from_timespec(struct timespec in);

#endif
