
#include "render_core.c"
#include "render_font.c"

#if OS_LINUX
# include "linux/render/linux_render.c"
# include "freetype/render_freetype.c"
#else
# error "platform render layer missing"
#endif

#if RENDER_BACKEND == RENDER_BACKEND_OPENGL
# include "opengl/render_opengl.c"
#else
# error "render backend missing"
#endif

