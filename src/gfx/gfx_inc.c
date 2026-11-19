
#include "gfx_core.c"

#if OS_LINUX
# include "linux/gfx/linux_gfx.c"
#elif OS_WINDOWS
# include "win32/gfx/win32_gfx.c"
#else
# error "platform gfx layer missing"
#endif
