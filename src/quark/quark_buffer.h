#ifndef QUARK_BUFFER_H
#define QUARK_BUFFER_H

#include "../base/base_inc.h"

typedef struct Buffer_ID Buffer_ID;
struct Buffer_ID {
	u32 index, generation_id;
};

typedef struct QBuffer QBuffer;

typedef struct QBuffer_List QBuffer_List;
struct QBuffer_List {
	QBuffer *first;
	usize len;
};

typedef struct QBuffer_Itr QBuffer_Itr;
struct QBuffer_Itr {
	QBuffer *buf;
	u8 *ptr;
	rune codepoint;
	usize pos;
	u8 size;
};

internal QBuffer *quark_buffer_new(Arena *arena, String8 data);
internal void     quark_buffer_clear(QBuffer *buf);

internal usize quark_buffer_length(QBuffer *buf);
internal String8 quark_buffer_slice(QBuffer *buf, usize begin, usize end, Arena *arena);
internal QBuffer_Itr quark_buffer_itr(QBuffer *buf, QBuffer_Itr *prev);

internal bool quark_buffer_insert(QBuffer *buf, String8 string);


#endif
