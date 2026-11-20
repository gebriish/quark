
#include "core.h"

#if OS_LINUX
# include "linux/gfx/inc.h"
#else
# error "platform gfx layer missing"
#endif
