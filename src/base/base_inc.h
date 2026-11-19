
#include "base_core.h"
#include "base_math.h"
#include "base_arena.h"
#include "base_string.h"
#include "base_threads.h"
#include "base_files.h"

#if OS_LINUX
# include "linux/base/linux_base.h"
#elif OS_WINDOWS
# include "win32/base/win32_base.h"
#else
# error "base layer platform missing"
#endif
