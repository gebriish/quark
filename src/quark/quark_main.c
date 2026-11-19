
//////////////////////
// @section : .h includes

#include "base/base_inc.h"
#include "gfx/gfx_inc.h"
#include "render/render_inc.h"
#include "ui/ui_inc.h"
#include "quark/quark_inc.h"

//////////////////////
// @section : .c includes

#include "base/base_inc.c"
#include "gfx/gfx_inc.c"
#include "render/render_inc.c"
#include "ui/ui_inc.c"
#include "quark/quark_inc.c"

function void
entry_point(Cmd_Line *cmd)
{
  q_init();

  for (b32 quit=0; !quit;)
  {
    quit = q_frame();
  }
}
