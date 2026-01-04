#ifndef OS_INC_H
#define OS_INC_H

#include "../base/base_context.h"

#include "os_core.h"

#if   OS_WINDOWS
# include "./win32/os_core_win32.h"
#elif OS_LINUX
# include "./linux/os_core_linux.h"
#else
# error "OS not supported"
#endif


#endif
