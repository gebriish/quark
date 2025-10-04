#include "gfx_font.h"


internal_lnk void
font_init(String8 path)
{
  Arena *arena = arena_alloc(MB(5));
  fnt_state = arena_push_struct(arena, Font_State);
  fnt_state->arena = arena;
  FT_Init_FreeType(&fnt_state->library);

  Temp scratch = temp_begin(arena);
  String8 c_path = str8_cstr_copy(arena, path);
  FT_Face face = 0;
  FT_New_Face(fnt_state->library, (char *)c_path.raw, 0, &face);
  temp_end(scratch);

  fnt_state->face = face;
}


