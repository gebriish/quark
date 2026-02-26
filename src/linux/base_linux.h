#ifndef BASE_LINX_H
#define BASE_LINX_H

#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>

#include "../base.h"

internal u64 os_linx_time_from_timespec(struct timespec in);
internal OS_FileProps  os_linx_file_props_from_stats(struct stat *s);

#endif
