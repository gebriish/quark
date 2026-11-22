
#include "core.c"
#include "font.c"
# include "freetype/core.c"

#if OS_LINUX
# include "linux/render/inc.c"
#else
# error "platform render layer missing"
#endif

#if RENDER_BACKEND == RENDER_BACKEND_OPENGL
# include "opengl/core.c"
#else
# error "render backend missing"
#endif

