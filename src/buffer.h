#ifndef BUFFER_H
#define BUFFER_H

///////////////////////////////////////////////////////////////////
// ~geb: This is the text buffer datastructure that is used by
//       quark to edit file data efficiently. All the api SHOULd
//       be designed in a way that internal implementation details
//       remain abstracted away.
///////////////////////////////////////////////////////////////////

#include "base.h"

typedef struct Q_Buffer Q_Buffer;

typedef struct {
	usize row;
	usize col;
} Buffer_Position;

typedef struct {
	usize offset;
	rune codepoint;
	Buffer_Position position;
	bool is_on_cursor;
} Q_Iterator;

internal bool buffer_iter(Q_Buffer *buf, Q_Iterator *itr);

internal Q_Buffer *buffer_new(String8 src, Q_Buffer *current, Allocator alloc);
internal void      buffer_delete(Q_Buffer *buffer);

internal Q_Buffer *buffer_insert(Q_Buffer *buf, String8 text);
internal void buffer_erase(Q_Buffer *buf);

internal void buffer_move_left(Q_Buffer *buffer);
internal void buffer_move_right(Q_Buffer *buffer);
internal void buffer_move_up(Q_Buffer *b);
internal void buffer_move_down(Q_Buffer *b);

internal Buffer_Position buffer_line_start(Q_Buffer *buffer);
internal Buffer_Position buffer_line_end(Q_Buffer *buffer);


#endif
