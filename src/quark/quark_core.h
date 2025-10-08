#ifndef QUARK_CORE_H
#define QUARK_CORE_H

#include "../base/base_core.h"
#include "../gfx/gfx_core.h"

////////////////////////////////
// ~geb: thirdparty includes
#define RGFW_OPENGL
#define RGFW_IMPLEMENTATION
#include "../thirdparty/rgfw.h"

typedef struct Quark_Context Quark_Context;
struct Quark_Context {
  Arena *persist_arena;
  Arena *transient_arena;
};

internal_lnk void quark_new(Quark_Context *context);
internal_lnk void quark_delete(Quark_Context *context);

#endif
