#ifndef BASE_THREADS_H
#define BASE_THREADS_H

/////////////////////////////
// @section: timing types

typedef U64 Time; // monotonic ns ( arbitrary origin ).

typedef struct TimeDuration TimeDuration;
struct TimeDuration
{
  F64 seconds;
  F64 milliseconds;
  F64 microseconds;
};

/////////////////////////////
// @section: thread context types

typedef struct ThreadCtx ThreadCtx;
struct ThreadCtx
{
  Arena *arenas[2];
};

/////////////////////////////
// @section: main thread types

typedef struct CmdLine CmdLine;
struct CmdLine
{
  String8 exe_name;   // just the executable name
  String8List args;   // arguments apart from the executable name
  String8List flags;  // flags that are in the command
  String8List inputs; // non flag inputs
};

/////////////////////////////
// @section: thread context procs

function ThreadCtx *tctx_alloc(void);
function void       tctx_release(ThreadCtx *tctx);
function void       tctx_select(ThreadCtx *tctx);
function ThreadCtx *tctx_selected(void);
function Arena     *tctx_get_scratch(Arena **conflicts, U64 count);

#define scratch_begin(conflicts, count) temp_begin(tctx_get_scratch((conflicts), (count)))
#define scratch_end(scratch) temp_end(scratch)
#define ScratchScope(_t, _ar, _n) for (Temp _t = scratch_begin(_ar, _n); (_t).arena; scratch_end(_t), (_t).arena = 0)

function S32 run_shell_command(String8List args);

/////////////////////////////
// @section: timing procs

function Time time_now(void);
function void time_sleep_ns(Time t);
function TimeDuration time_diff(Time begin, Time end);

/////////////////////////////
// @section: main thread procs

function void main_thread_base_entry_point(int argc, char **argv);

#endif // BASE_THREADS_H
