
#include "core.c"

#if OS_LINUX
# include "linux/gfx/inc.c"
#else
# error "platform gfx layer missing"
#endif
