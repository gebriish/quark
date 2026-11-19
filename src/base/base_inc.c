
#include "base_core.c"
#include "base_math.c"
#include "base_arena.c"
#include "base_string.c"
#include "base_threads.c"
#include "base_files.c"

#if OS_LINUX
# include "linux/base/linux_base.c"
#elif OS_WINDOWS
# include "win32/base/win32_base.c"
#else
# error "base layer platform missing"
#endif
