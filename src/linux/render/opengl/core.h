
#define glTexSubImage2D glTexSubImage2D__static
#define glActiveTexture glActiveTexture__static
#include <GL/gl.h>
#include <GL/glx.h>
#undef glTexSubImage2D
#undef glActiveTexture


#define GLX_CONTEXT_MAJOR_VERSION_ARB          0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB          0x2092
#define GLX_CONTEXT_FLAGS_ARB                  0x2094
#define GLX_CONTEXT_DEBUG_BIT_ARB              0x00000001
#define GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB 0x00000002
typedef GLXContext (*glXCreateContextAttribsARBProc)(Display*, GLXFBConfig, GLXContext, Bool, const int*);
typedef void (*R_GLXSwapIntervalEXT)(Display *, GLXDrawable, int);

global GLXContext opengl_lnx_ctx = 0;
global R_GLXSwapIntervalEXT r_glXSwapIntervalEXT = 0;

