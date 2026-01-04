#include "quark_buffer.h"

struct QBuffer {
	QBuffer *next;

	usize gap_index;
	usize gap_size;
	usize capacity;
	u8    bytes[];
};

force_inline usize
_qb_logical_to_physical_idx(QBuffer *buf, usize logical)
{
	usize physical = 0;
	if (logical < buf->gap_index) 
		physical = logical;
	else
		physical = logical + buf->gap_size;
	return physical;
}

internal QBuffer *
quark_buffer_new(Arena *arena, String8 data)
{
	Assert(arena);
	
	usize usable_cap = data.len * 2;
	
	if (usable_cap < KB(64)) {
		usable_cap = AlignPow2(Max(usable_cap, KB(4)), KB(4));
	} else if (usable_cap < MB(1)) {
		usable_cap = AlignPow2(usable_cap, KB(64));
	} else {
		usable_cap = AlignPow2(usable_cap, MB(1));
	}
	
	usize total_cap = sizeof(QBuffer) + usable_cap;
	u8 *memory = arena_push_array(arena, u8, total_cap);
	QBuffer *buf = (QBuffer *)memory;
	
	buf->next       = NULL;
	buf->capacity   = usable_cap;
	buf->gap_index  = 0;
	buf->gap_size   = usable_cap - data.len;
	
	MemMove(buf->bytes + buf->gap_size, data.str, data.len);
	
	return buf;
}

internal void
quark_buffer_clear(QBuffer *buf)
{
	Assert(buf);

	MemZero(buf->bytes, buf->capacity);
	buf->gap_size = buf->capacity;
	buf->gap_index = 0;
}

internal usize
quark_buffer_length(QBuffer *buf)
{
	Assert(buf);
	return buf->capacity - buf->gap_size;
}


internal String8
quark_buffer_slice(QBuffer *buf, usize begin, usize end, Arena *arena)
{
	Assert(buf);
	Assert(begin <= end);
	
	usize slice_size = end - begin;
	if (slice_size == 0) {
		return str8(NULL, 0);
	}
	
	u8 *dst = arena_push_array(arena, u8, slice_size);
	usize gap_idx = buf->gap_index;
	
	if (end <= gap_idx) {
		MemMove(dst, buf->bytes + begin, slice_size);
	}
	else if (begin >= gap_idx) {
		MemMove(dst, buf->bytes + begin + buf->gap_size, slice_size);
	}
	else {
		usize left_size = gap_idx - begin;
		MemMove(dst, buf->bytes + begin, left_size);
		MemMove(dst + left_size, buf->bytes + gap_idx + buf->gap_size, slice_size - left_size);
	}
	
	return str8(dst, slice_size);
}

internal QBuffer_Itr
quark_buffer_itr(QBuffer *buf, QBuffer_Itr *prev)
{
	QBuffer_Itr itr = {0};
	
	if (prev) {
		usize consumed = UTF8_SIZE[*prev->ptr];
		if (!consumed) consumed = 1;
		
		usize next_phy = prev->ptr - buf->bytes + consumed;
		
		if (next_phy == buf->gap_index) {
			next_phy += buf->gap_size;
		}
		
		if (next_phy >= buf->capacity) return itr;
		
		itr.pos = prev->pos + 1;
		itr.ptr = buf->bytes + next_phy;
	} else {
		if (buf->capacity == buf->gap_size) return itr;

		itr.pos = 0;
		itr.ptr = buf->bytes + ((buf->gap_index == 0) * buf->gap_size);
	}
	
	u8 first = *itr.ptr;
	if (first < 0x80) {
		itr.codepoint = first;
		return itr;
	}
	
	usize _;
	if (utf8_decode(str8(itr.ptr, 4), 0, &itr.codepoint, &_) != UTF8_Err_None) {
		itr.codepoint = Utf8_Invalid;
	}
	
	return itr;
}


internal bool
quark_buffer_insert(QBuffer *buf, String8 string)
{
	Assert(buf);

	usize len = quark_buffer_length(buf);
	if (len + string.len > buf->capacity) return false;

	u8 *buffer_dest = buf->bytes + buf->gap_index;

	MemMove(buffer_dest, string.str, string.len);

	buf->gap_index += string.len;
	buf->gap_size -= string.len;

	return true;
}
