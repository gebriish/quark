
#include "core.c"
#include "math.c"
#include "arena.c"
#include "string.c"
#include "threads.c"
#include "files.c"

#if OS_LINUX
# include "linux/base/inc.c"
#else
# error "base layer platform missing"
#endif
