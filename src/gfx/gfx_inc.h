
#include "gfx_core.h"

#if OS_LINUX
# include "linux/gfx/linux_gfx.h"
#elif OS_WINDOWS
# include "win32/gfx/win32_gfx.h"
#else
# error "platform gfx layer missing"
#endif
