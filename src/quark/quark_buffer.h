#ifndef QUARK_BUFFER_H
#define QUARK_BUFFER_H

#include "../base/base_core.h"
#include "../base/base_string.h"

/////////////////////////////////////
// ~geb: Gap buffer

typedef struct Quark_Buffer Quark_Buffer;
struct Quark_Buffer {
	u8 *data;

	usize capacity;
	usize gap_index;
	usize gap_size;

	Quark_Buffer *next; // linked list of buffers
};

typedef u32 Cursor_Dir;
enum {
	Cursor_Dir_Left,
	Cursor_Dir_Right,
	Cursor_Dir_Up,
	Cursor_Dir_Down,
};

internal void q_buffer_insert(Quark_Buffer *buffer, String8 string);
internal void q_buffer_delete_rune(Quark_Buffer *buffer, u32 count, Cursor_Dir dir);
internal void q_buffer_move_gap(Quark_Buffer *buffer, usize byte);
internal void q_buffer_move_gap_by(Quark_Buffer *buffer, u32 count, Cursor_Dir dir);

internal String8 q_buffer_to_str(Quark_Buffer *buffer, Arena *allocator);

internal u32  runes_till(Quark_Buffer *buffer, rune target);
internal u32  runes_from(Quark_Buffer *buffer, rune target);

/////////////////////////////////////
// ~geb: Gap buffer Manager

typedef struct Buffer_Manager Buffer_Manager;
struct Buffer_Manager {
	Arena *allocator;

	Quark_Buffer *buffer_list;
	Quark_Buffer *buffer_freelist;
};

internal void buffer_manager_init(Arena *allocator, Buffer_Manager *bm);
internal void buffer_manager_clear(Buffer_Manager *bm);

internal Quark_Buffer *q_buffer_new(Buffer_Manager *bm, usize buffer_size);
internal void q_buffer_delete(Buffer_Manager *bm, Quark_Buffer *buffer);

#endif
