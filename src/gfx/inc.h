#ifndef GFX_INC_H
#define GFX_INC_H

#include "core.h"

#if OS_LINUX
# include "linux/gfx/inc.h"
#else
# error "platform gfx layer missing"
#endif

#endif // GFX_INC_H
