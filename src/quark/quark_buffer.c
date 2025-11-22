#include "quark_buffer.h"
#include <stdlib.h>


/////////////////////////////////////

internal void 
q_buffer_insert(Quark_Buffer *buffer, String8 string)
{
	Assert(buffer && buffer->data);
	if (string.len == 0) return;

	if (string.len > buffer->gap_size) {
		usize new_capacity = Max(buffer->capacity * 2, buffer->capacity + string.len);
		u8 *new_data = malloc(new_capacity);

		MemMove(new_data, buffer->data, buffer->gap_index);

		usize after_gap_len = buffer->capacity - (buffer->gap_index + buffer->gap_size);
		MemMove(new_data + new_capacity - after_gap_len, 
					buffer->data + buffer->gap_index + buffer->gap_size, 
					after_gap_len);

		buffer->gap_size = new_capacity - buffer->gap_index - after_gap_len;

		free(buffer->data);
		buffer->data = new_data;
		buffer->capacity = new_capacity;
	}

	usize gap_start = buffer->gap_index;
	MemMove(buffer->data + gap_start, string.raw, string.len);
	buffer->gap_index += string.len;
	buffer->gap_size -= string.len;
}

internal void
q_buffer_move_gap_by(Quark_Buffer *buffer, u32 count, Cursor_Dir dir)
{
	Assert(buffer && buffer->data);

	usize gap_start = buffer->gap_index;
	usize gap_end = gap_start + buffer->gap_size;

	if (dir == Cursor_Dir_Left) {
		usize new_gap_pos = gap_start;
		while (count > 0 && new_gap_pos > 0) {
			new_gap_pos -= 1;
			while (new_gap_pos > 0 && utf8_trail(buffer->data[new_gap_pos])) {
				new_gap_pos -= 1;
			}
			count -= 1;
		}
		q_buffer_move_gap(buffer, new_gap_pos);
	}
	else if (dir == Cursor_Dir_Right) {
		usize new_gap_pos = gap_start;
		while (count > 0 && gap_end < buffer->capacity) {
			gap_end += 1;
			new_gap_pos += 1;
			while (gap_end < buffer->capacity && utf8_trail(buffer->data[gap_end])) {
				gap_end += 1;
				new_gap_pos += 1;
			}
			count -= 1;
		}
		q_buffer_move_gap(buffer, new_gap_pos);
	}
}


internal void
q_buffer_delete_rune(Quark_Buffer *buffer, u32 count, Cursor_Dir dir)
{
	Assert(buffer && buffer->data);
	while (count > 0) {
		if (dir == Cursor_Dir_Left) {
			if (buffer->gap_index == 0) break;
			do {
				buffer->gap_index -= 1;
				buffer->gap_size += 1;
			} while (buffer->gap_index > 0 &&
			         utf8_trail(buffer->data[buffer->gap_index]));
		}
		else if (dir == Cursor_Dir_Right) {
			usize after_gap = buffer->gap_index + buffer->gap_size;
			if (after_gap >= buffer->capacity) break;
			do {
				buffer->gap_size += 1;
			} while (buffer->gap_index + buffer->gap_size < buffer->capacity &&
			         utf8_trail(buffer->data[buffer->gap_index + buffer->gap_size]));
		}
		count -= 1;
	}
}

internal void
q_buffer_move_gap(Quark_Buffer *buffer, usize byte_index)
{
	Assert(buffer && buffer->data);
	
	usize gap_start = buffer->gap_index;
	usize gap_end_exclusive = gap_start + buffer->gap_size;
	
	if (byte_index > buffer->capacity - buffer->gap_size) { 
		return; 
	}
	
	if (byte_index < gap_start) {
		usize move_size = gap_start - byte_index;
		MemMove(buffer->data + gap_end_exclusive - move_size, 
					buffer->data + byte_index, 
					move_size);
		buffer->gap_index = byte_index;
	} else { // byte_index >= gap_end_exclusive
		usize move_size = byte_index - gap_start;
		MemMove(buffer->data + gap_start, 
					buffer->data + gap_end_exclusive, 
					move_size);
		buffer->gap_index = byte_index;
	}
}


internal String8
q_buffer_to_str(Quark_Buffer *buffer, Arena *allocator)
{
	usize str_len = buffer->capacity - buffer->gap_size;
	if (str_len <= 0) return str8(NULL, 0);

	u8 *result = arena_push_array(allocator, u8, str_len);
	if (!result) {
		LogError("Failed to allocate memory for buffer string conversion");
		return str8(NULL, 0);
	}

	usize before_gap_len = buffer->gap_index;
	MemMove(result, buffer->data, before_gap_len);

	usize gap_end = buffer->gap_index + buffer->gap_size;
	usize after_gap_len = buffer->capacity - gap_end;
	MemMove(result + before_gap_len, buffer->data + gap_end, after_gap_len);

	return str8(result, str_len);
}

internal u32
runes_till(Quark_Buffer *buffer, rune target)
{
	Assert(buffer && buffer->data);
	
	usize gap_end = buffer->gap_index + buffer->gap_size;
	if (gap_end >= buffer->capacity) { return 0; }

	u8 *string_begin = buffer->data + gap_end;

	String8 slice = str8(string_begin, buffer->capacity - gap_end);

	u32 rune_count = 0;
	str8_foreach(slice, itr, i) {
		if (itr.codepoint == target) {
			goto outside;
		}
		rune_count += 1;
	}

outside:
	return rune_count;
}

internal u32
runes_from(Quark_Buffer *buffer, rune target)
{
	Assert(buffer && buffer->data);
	
	if (buffer->gap_index == 0) { return 0; }
	
	u8 *string_begin = buffer->data;
	String8 slice = str8(string_begin, buffer->gap_index);
	
	u32 rune_count = 0;
	
	usize pos = slice.len;
	while (pos > 0) {
		pos -= 1;
		while (pos > 0 && utf8_trail(slice.raw[pos])) {
			pos -= 1;
		}
		
		u8 *rune_start = slice.raw + pos;
		usize remaining = slice.len - pos;
		rune_itr decoded = str8_decode_utf8(rune_start, remaining);
		
		if (decoded.codepoint == target) {
			goto outside;
		}
		
		rune_count += 1;
	}
	
outside:
	return rune_count;
}

/////////////////////////////////////

internal void 
buffer_manager_init(Arena *allocator, Buffer_Manager *bm)
{
	Assert(bm && "Cannot Init null buffer manager");
	bm->allocator = allocator;
	bm->buffer_list = NULL;
	bm->buffer_freelist = NULL;
}

internal Quark_Buffer *
q_buffer_new(Buffer_Manager *bm, usize buffer_size)
{
	Assert(bm && "Cannot add buffer into null buffer manager");
	Quark_Buffer *new_buffer = NULL;
	bool reused = false;
	
	if (bm->buffer_freelist) {
		new_buffer = bm->buffer_freelist;
		bm->buffer_freelist = bm->buffer_freelist->next;
		reused = true;
	} else {
		new_buffer = arena_push_struct(bm->allocator, Quark_Buffer);
	}
	
	if (!new_buffer) {
		LogError("Ran out of buffer capacity, consider closing some buffers");
		return NULL;
	}
	
	if (reused) {
		if (new_buffer->capacity < buffer_size) {
			u8 *new_data = (u8 *)realloc(new_buffer->data, buffer_size);
			if (!new_data) {
				LogError("Failed to reallocate gap buffer data");
				new_buffer->next = bm->buffer_freelist;
				bm->buffer_freelist = new_buffer;
				return NULL;
			}
			new_buffer->data = new_data;
			new_buffer->capacity = buffer_size;
		}
	} else {
		new_buffer->data = (u8 *)malloc(buffer_size);
		if (!new_buffer->data) {
			LogError("Failed to allocate gap buffer data");
			new_buffer->next = bm->buffer_freelist;
			bm->buffer_freelist = new_buffer;
			return NULL;
		}
		new_buffer->capacity = buffer_size;
	}
	
	new_buffer->gap_index = 0;
	new_buffer->gap_size = new_buffer->capacity;
	
	new_buffer->next = bm->buffer_list;
	bm->buffer_list = new_buffer;
	return new_buffer;
}

internal void
q_buffer_delete(Buffer_Manager *bm, Quark_Buffer *buffer)
{
	Assert(bm && "Cannot delete buffer from null buffer manager");
	Assert(buffer && "Cannot delete null buffer");
	
	Quark_Buffer *curr_buff = bm->buffer_list;
	Quark_Buffer *prev = NULL;
	
	while(curr_buff != NULL && curr_buff != buffer) {
		prev = curr_buff;
		curr_buff = curr_buff->next;
	}
	
	if (!curr_buff) { 
		LogError("Trying to delete buffer that is not in Buffer Manager");
		return; 
	}
	
	if (prev) {
		prev->next = curr_buff->next;
	} else {
		bm->buffer_list = curr_buff->next;
	}
	
	curr_buff->gap_index = 0;
	curr_buff->gap_size = curr_buff->capacity;
	
	curr_buff->next = bm->buffer_freelist;
	bm->buffer_freelist = curr_buff;
}

internal void
buffer_manager_clear(Buffer_Manager *bm)
{
	Assert(bm && "Cannot clear null buffer manager");
	
	Quark_Buffer *curr = bm->buffer_list;
	while (curr) {
		Quark_Buffer *next = curr->next;

		curr->gap_index = 0;
		curr->gap_size = curr->capacity;

		curr->next = bm->buffer_freelist;
		bm->buffer_freelist = curr;

		curr = next;
	}
	
	bm->buffer_list = NULL;
}
