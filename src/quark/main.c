#include "base/inc.h"
#include "gfx/inc.h"
#include "render/inc.h"
#include "ui/ui_inc.h"
#include "quark/inc.h"

#include "base/inc.c"
#include "gfx/inc.c"
#include "render/inc.c"
#include "ui/ui_inc.c"
#include "quark/inc.c"

function void
entry_point(Cmd_Line *cmd)
{
  q_init();

  for (b32 quit=0; !quit;)
  {
    quit = q_frame();
  }
}
