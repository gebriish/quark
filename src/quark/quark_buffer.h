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
	usize gap_index;
	usize gap_size;

	Gap_Buffer *next; // linked list of buffers
};

typedef u32 Cursor_Dir;
enum {
	Cursor_Dir_Left,
	Cursor_Dir_Right,
	Cursor_Dir_Up,
	Cursor_Dir_Down,
};

internal void gap_buffer_insert(Gap_Buffer *buffer, String8 string);
internal void gap_buffer_delete_rune(Gap_Buffer *buffer, u32 count, Cursor_Dir dir);
internal void gap_buffer_move_gap(Gap_Buffer *buffer, usize byte);
internal void gap_buffer_move_gap_by(Gap_Buffer *buffer, u32 count, Cursor_Dir dir);

internal String8 gap_buffer_to_str(Gap_Buffer *buffer, Arena *allocator);

internal u32  runes_till(Gap_Buffer *buffer, rune target);
internal u32  runes_from(Gap_Buffer *buffer, rune target);

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
