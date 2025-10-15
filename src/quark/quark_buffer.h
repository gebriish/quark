#ifndef QUARK_BUFFER_H
#define QUARK_BUFFER_H

#include "../base/base_core.h"
#include "../base/base_string.h"


/////////////////////////////////////
// ~geb: Gap buffer

typedef struct Gap_Buffer Gap_Buffer;
struct Gap_Buffer {
	u8 *data;

	usize capacity;
	usize gap_size;
	usize cursor_pos;

	Gap_Buffer *next; // linked list of buffers
};

typedef u32 Cursor_Move; 
enum {
	Cursor_Move_Left,
	Cursor_Move_Right,
	Cursor_Move_Down,
	Cursor_Move_Up,
};

internal void gap_buffer_insert(Gap_Buffer *buffer, String8 string);
internal void gap_buffer_move_cursor(Gap_Buffer *buffer, Cursor_Move direction, int count);


/////////////////////////////////////
// ~geb: Gap buffer Manager

typedef struct Buffer_Manager Buffer_Manager;
struct Buffer_Manager {
	Arena *allocator;

	Gap_Buffer *buffer_list;
	Gap_Buffer *buffer_freelist;
};

internal void buffer_manager_init(Arena *allocator, Buffer_Manager *bm);
internal void buffer_manager_clear(Buffer_Manager *bm);

internal Gap_Buffer *gap_buffer_new(Buffer_Manager *bm, usize buffer_size);
internal void        gap_buffer_delete(Buffer_Manager *bm, Gap_Buffer *buffer);

#endif
