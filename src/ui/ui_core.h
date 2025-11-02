#ifndef UI_CORE_H
#define UI_CORE_H

#include "../base/base_core.h"

typedef struct UI_Context UI_Context;
struct UI_Context {
	Arena *internal_arena;
};

typedef struct UI_Box UI_Box;
struct UI_Box {
	
};

internal UI_Context *ui_init(Arena *allocator);
internal UI_Context *ui_set_context(UI_Context *ctx);

#endif
