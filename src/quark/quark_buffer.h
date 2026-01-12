#ifndef QUARK_BUFFER_H
#define QUARK_BUFFER_H

#include "../base/base_inc.h"

typedef struct QBuffer QBuffer;
struct QBuffer {
	String8 name;

	QBuffer *next;
	QBuffer *prev;

	usize gap_index;
	usize gap_size;
	usize capacity;
	u8    bytes[];
};

typedef struct QBuffer_Itr QBuffer_Itr;
struct QBuffer_Itr {
	u8   *ptr;
	usize pos;
	u32  codepoint;
	bool on_cursor;
};

internal QBuffer *quark_buffer_new(Arena *arena, String8 data);
internal void     quark_buffer_clear(QBuffer *buf);

internal usize quark_buffer_length(QBuffer *buf);
internal String8 quark_buffer_slice(QBuffer *buf, usize begin, usize end, Arena *arena);
internal QBuffer_Itr quark_buffer_itr(QBuffer *buf, QBuffer_Itr *prev);

internal bool quark_buffer_insert(QBuffer *buf, String8 string, usize cursor);
internal void quark_buffer_delete(QBuffer *buf, usize count, usize cursor, bool backspace);

typedef struct QBuffer_List QBuffer_List;
struct QBuffer_List {
	QBuffer *first;
	QBuffer *last;
	usize len;
};

internal void qbuffer_list_push(QBuffer_List *list, QBuffer *buf);
internal bool qbuffer_list_remove(QBuffer_List *list, QBuffer *buf);

#endif
