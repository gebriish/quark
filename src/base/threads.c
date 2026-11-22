
/////////////////////////////
// @section: thread context

thread_static Thread_Ctx *tctx_thread_local = 0;

function Thread_Ctx *
tctx_alloc(void)
{
  Arena *arena_0 = arena_reserve(Gigabytes(2));
  Arena *arena_1 = arena_reserve(Gigabytes(2));

  Thread_Ctx *tctx = arena_push(arena_0, Thread_Ctx, 1);
  tctx->arenas[0] = arena_0;
  tctx->arenas[1] = arena_1;

  return tctx;
}

function void
tctx_release(Thread_Ctx *tctx)
{
  arena_release(tctx->arenas[1]);
  arena_release(tctx->arenas[0]);
}

function void
tctx_select(Thread_Ctx *tctx)
{
  tctx_thread_local = tctx;
}

function Thread_Ctx *
tctx_selected(void)
{
  return tctx_thread_local;
}

function Arena *
tctx_get_scratch(Arena **conflicts, u64 count)
{
  Thread_Ctx *tctx = tctx_selected();
  Arena *result = 0;
  Arena **arena_ptr = tctx->arenas;
  for each_index(i, CArrayLength(tctx->arenas))
  {
    Arena **conflict_ptr = conflicts;
    b32 has_conflict = 0;
    for(u64 j = 0; j < count; j += 1, conflict_ptr += 1)
    {
      if(*arena_ptr == *conflict_ptr)
      {
        has_conflict = 1;
        break;
      }
    }
    if(!has_conflict)
    {
      result = *arena_ptr;
      break;
    }
    arena_ptr += 1;
  }
  return result;
}

/////////////////////////////
// @section: timing

function Time_Duration
time_diff(Time begin, Time end)
{
  Time ns = end - begin;

  Time_Duration result = {
    .seconds      = (f64) ns / 1e9,
    .milliseconds = (f64) ns / 1e6,
    .microseconds = (f64) ns / 1e3
  };

  return result;
}

/////////////////////////////
// @section: main thread

function void entry_point(Cmd_Line *cmd); // implemented by target

function void
main_thread_base_entry_point(int argc, char **argv)
{
  Cmd_Line cmd = {0};
  cmd.exe_name = str8_cstring((u8 *) argv[0]);

  Temp scratch = scratch_begin(0, 0);
  for each_index(i, argc)
  {
    String8 arg = str8_cstring((u8 *) argv[i]);
    String8_List *category = 0;

    switch (arg.raw[0])
    {
      case '-':
        category = &cmd.flags;
        break;

      default :
        category = &cmd.inputs;
        break;
    }

    str8_list_push(scratch.arena, category, arg);
    str8_list_push(scratch.arena, &cmd.args, arg);
  }

#if defined(GFX_CORE_H)
  gfx_init();
#endif

#if defined(RENDER_CORE_H)
  rndr_init();
#endif

#if defined(UI_CORE_H)
  ui_init();
#endif

  entry_point(&cmd);
}
