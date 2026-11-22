// rn Im only gonna support OpenGL

#define RENDER_BACKEND_OPENGL 1
#define RENDER_BACKEND_D3D11  2
#define RENDER_BACKEND_METAL  3

#if !defined(RENDER_BACKEND)
# define RENDER_BACKEND RENDER_BACKEND_OPENGL 
#endif

#include "core.h"
#include "font.h"
#include "freetype/core.h"

#if OS_LINUX
# include "linux/render/inc.h"
#else
# error "platform render layer missing"
#endif

#if RENDER_BACKEND == RENDER_BACKEND_OPENGL
# include "opengl/core.h"
#else
# error "render backend missing"
#endif
