#include "../base/base_context.h"

#include "os_core.c"

#if OS_LINUX
# include "./linux/os_core_linux.c"
#endif
