#include "quark_buffer.h"

////////////////////////////////
// ~geb: Gap buffer specific helpers

internal usize
qb_logical_to_physical(QBuffer *b, usize logical)
{
	Assert(logical <= quark_buffer_length(b));
	if (logical < b->gap_index) return logical;
	return logical + b->gap_size;
}

internal void
qb_move_gap(QBuffer *b, usize logical)
{
	Assert(logical <= quark_buffer_length(b));

	if (logical == b->gap_index) return;

	usize phys = qb_logical_to_physical(b, logical);

	if (logical < b->gap_index) {
		usize count = b->gap_index - logical;
		MemMove(
			b->bytes + phys + b->gap_size,
			b->bytes + phys,
			count
		);
	} else {
		usize count = logical - b->gap_index;
		MemMove(
			b->bytes + b->gap_index,
			b->bytes + b->gap_index + b->gap_size,
			count
		);
	}

	b->gap_index = logical;
}

////////////////////////////////

internal QBuffer *
quark_buffer_new(Arena *arena, String8 data)
{
	Assert(arena);

	usize cap = Max(KB(4), AlignPow2(data.len * 2, KB(4)));
	u8 *mem = arena_push_array(arena, u8, sizeof(QBuffer) + cap);

	QBuffer *b = (QBuffer *)mem;
	b->capacity  = cap;
	b->gap_index = 0;
	b->gap_size  = cap - data.len;
	b->next = b->prev = NULL;
	b->name = S("");

	MemMove(b->bytes + b->gap_size, data.str, data.len);
	return b;
}

internal void
quark_buffer_clear(QBuffer *b)
{
	b->gap_index = 0;
	b->gap_size  = b->capacity;
}

internal usize
quark_buffer_length(QBuffer *b)
{
	Assert(b);
	return b->capacity - b->gap_size;
}

internal usize
quark_buffer_capacity(QBuffer *buf) 
{
	Assert(buf);
	return buf->capacity;
}


internal String8
quark_buffer_slice(QBuffer *b, usize begin, usize end, Arena *arena)
{
	Assert(begin <= end);
	Assert(end <= quark_buffer_length(b));

	usize len = end - begin;
	if (!len) return str8(0, 0);

	u8 *dst = arena_push_array(arena, u8, len);

	usize p0 = qb_logical_to_physical(b, begin);
	usize p1 = qb_logical_to_physical(b, end);

	if (p0 < b->gap_index && p1 <= b->gap_index) {
		MemMove(dst, b->bytes + p0, len);
	}
	else if (p0 >= b->gap_index + b->gap_size) {
		MemMove(dst, b->bytes + p0, len);
	}
	else {
		usize left = b->gap_index - p0;
		MemMove(dst, b->bytes + p0, left);
		MemMove(dst + left,
					b->bytes + b->gap_index + b->gap_size,
					len - left);
	}

	return str8(dst, len);
}

internal QBuffer_Itr
quark_buffer_itr(QBuffer *b, QBuffer_Itr *prev)
{
	QBuffer_Itr it = {0};

	usize logical = prev ? prev->pos + 1 : 0;
	if (logical >= quark_buffer_length(b)) return it;

	usize phys = qb_logical_to_physical(b, logical);
	u8 *ptr = b->bytes + phys;

	it.pos = logical;
	it.ptr = ptr;
	it.on_cursor = (logical == b->gap_index);

	u8 first = *ptr;
	if (first < 0x80) {
		it.codepoint = first;
		return it;
	}

	usize used = 0;
	if (utf8_decode(str8(ptr, 4), 0, &it.codepoint, &used) != UTF8_Err_None) {
		it.codepoint = Utf8_Invalid;
	}

	return it;
}


internal bool
quark_buffer_insert(QBuffer *b, String8 s, usize cursor)
{
	Assert(cursor <= quark_buffer_length(b));

	if (s.len > b->gap_size) return false;

	qb_move_gap(b, cursor);
	MemMove(b->bytes + b->gap_index, s.str, s.len);

	b->gap_index += s.len;
	b->gap_size  -= s.len;
	return true;
}

internal void
quark_buffer_delete(QBuffer *b, usize count, usize cursor, bool backspace)
{
	if (!count) return;

	usize start = cursor;
	if (backspace) {
		if (!cursor) return;
		count = Min(count, cursor);
		start = cursor - count;
	}

	usize avail = quark_buffer_length(b) - start;
	count = Min(count, avail);

	qb_move_gap(b, start);
	b->gap_size += count;
}


/////////////////////////////////////////////
// ~geb: Quark Buffer List procs

internal void
qbuffer_list_push(QBuffer_List *list, QBuffer *buf)
{
	Assert(list);
	Assert(buf);

	buf->next = NULL;
	buf->prev = list->last;

	if (list->last) {
		list->last->next = buf;
	} else {
		list->first = buf;
	}

	list->last = buf;
	list->len += 1;
}


internal bool
qbuffer_list_remove(QBuffer_List *list, QBuffer *buf)
{
	Assert(list);
	Assert(buf);

	if (!buf->prev && !buf->next && list->first != buf) {
		return false;
	}

	if (buf->prev) {
		buf->prev->next = buf->next;
	} else {
		list->first = buf->next;
	}

	if (buf->next) {
		buf->next->prev = buf->prev;
	} else {
		list->last = buf->prev;
	}

	buf->next = NULL;
	buf->prev = NULL;
	list->len -= 1;

	return true;
}
