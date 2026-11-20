function Time 
time_now(void)
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

function void 
time_sleep_ns(Time ns)
{
  struct timespec ts = {0};
  ts.tv_sec = ns / 1000000000LL;
  ts.tv_nsec = ns % 1000000000LL; 
  nanosleep(&ts, 0);
}

int 
main(int argc, char **argv)
{
  Thread_Ctx *tctx = tctx_alloc();
  tctx_select(tctx);
  main_thread_base_entry_point(argc, argv);
}
