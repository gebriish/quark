
#include "../base/base_context.h"

#include "os_core.c"

#if   OS_WINDOWS
# include "./win32/os_core_win32.c"
#elif OS_LINUX
# include "./linux/os_core_linux.c"
#else
# error "OS not supported"
#endif
