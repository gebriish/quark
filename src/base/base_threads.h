#ifndef BASE_THREADS_H
#define BASE_THREADS_H

/////////////////////////////
// @section: timing types

typedef u64 Time; // monotonic ns ( arbitrary origin ).

typedef struct Time_Duration Time_Duration;
struct Time_Duration
{
  f64 seconds;
  f64 milliseconds;
  f64 microseconds;
};

/////////////////////////////
// @section: thread context types

typedef struct Thread_Ctx Thread_Ctx;
struct Thread_Ctx
{
  Arena *arenas[2];
};

/////////////////////////////
// @section: main thread types

typedef struct Cmd_Line Cmd_Line;
struct Cmd_Line
{
  String8 exe_name;   // just the executable name
  String8_List args;   // arguments apart from the executable name
  String8_List flags;  // flags that are in the command
  String8_List inputs; // non flag inputs
};

/////////////////////////////
// @section: thread context procs

function Thread_Ctx *tctx_alloc(void);
function void        tctx_release(Thread_Ctx *tctx);
function void        tctx_select(Thread_Ctx *tctx);
function Thread_Ctx *tctx_selected(void);
function Arena      *tctx_get_scratch(Arena **conflicts, u64 count);

#define scratch_begin(conflicts, count) temp_begin(tctx_get_scratch((conflicts), (count)))
#define scratch_end(scratch) temp_end(scratch)
#define ScratchScope(_t, _ar, _n) for (Temp _t = scratch_begin(_ar, _n); (_t).arena; scratch_end(_t), (_t).arena = 0)

/////////////////////////////
// @section: timing procs

function Time time_now(void);
function void time_sleep_ns(Time t);
function Time_Duration time_diff(Time begin, Time end);

/////////////////////////////
// @section: main thread procs

function void main_thread_base_entry_point(int argc, char **argv);

#endif // BASE_THREADS_H
