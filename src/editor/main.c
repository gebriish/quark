#include "base/inc.h"
#include "gfx/inc.h"
#include "render/inc.h"
#include "ui/inc.h"
#include "editor/inc.h"

#include "base/inc.c"
#include "gfx/inc.c"
#include "render/inc.c"
#include "ui/inc.c"
#include "editor/inc.c"

function void
entry_point(Cmd_Line *cmd)
{
  editor_init(cmd);
  Arena *arena = editor_state->arena;

  for (b32 quit=0; !quit;)
  {
    quit = editor_frame();
  }
}
