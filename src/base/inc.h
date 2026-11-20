#ifndef BASE_INC_H
#define BASE_INC_H

#include "core.h"
#include "math.h"
#include "arena.h"
#include "string.h"
#include "threads.h"
#include "files.h"

#if OS_LINUX
# include "linux/base/inc.h"
#else
# error "base layer platform missing"
#endif

#endif // BASE_INC_H
