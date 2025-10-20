#include "quark_buffer.h"
#include <stdlib.h>


/////////////////////////////////////

internal void 
gap_buffer_insert(Gap_Buffer *buffer, String8 string)
{
	Assert(buffer && buffer->data);
	if (string.len == 0) return;

	if (string.len > buffer->gap_size) {
		usize new_capacity = buffer->capacity * 2;
		buffer->data = realloc(buffer->data, new_capacity);
		return;
	}

	usize gap_start = buffer->gap_index;
	usize gap_end   = gap_start + buffer->gap_size;

	MemMove(buffer->data + gap_start, string.raw, string.len);
	buffer->gap_index   += string.len;
	buffer->gap_size    -= string.len;
}


internal void
gap_buffer_delete_rune(Gap_Buffer *buffer, u32 count)
{
	Assert(buffer && buffer->data);
	
	while(count > 0 && buffer->gap_index > 0) {
		do {
			buffer->gap_index -= 1;
			buffer->gap_size += 1;
		} while(utf8_trail(buffer->data[buffer->gap_index]));
		count -= 1;
	}
}

internal void
gap_buffer_move_gap(Gap_Buffer *buffer, usize byte_index)
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

/////////////////////////////////////

internal void 
buffer_manager_init(Arena *allocator, Buffer_Manager *bm)
{
	Assert(bm && "Cannot Init null buffer manager");
	bm->allocator = arena_nest(allocator, KB(64));
	bm->buffer_list = NULL;
	bm->buffer_freelist = NULL;
}

internal Gap_Buffer *
gap_buffer_new(Buffer_Manager *bm, usize buffer_size)
{
	Assert(bm && "Cannot add buffer into null buffer manager");
	Gap_Buffer *new_buffer = NULL;
	bool reused = false;
	
	if (bm->buffer_freelist) {
		new_buffer = bm->buffer_freelist;
		bm->buffer_freelist = bm->buffer_freelist->next;
		reused = true;
	} else {
		new_buffer = arena_push_struct(bm->allocator, Gap_Buffer);
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
gap_buffer_delete(Buffer_Manager *bm, Gap_Buffer *buffer)
{
	Assert(bm && "Cannot delete buffer from null buffer manager");
	Assert(buffer && "Cannot delete null buffer");
	
	Gap_Buffer *curr_buff = bm->buffer_list;
	Gap_Buffer *prev = NULL;
	
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
	
	Gap_Buffer *curr = bm->buffer_list;
	while (curr) {
		Gap_Buffer *next = curr->next;
		
		curr->gap_index = 0;
		curr->gap_size = curr->capacity;
		
		curr->next = bm->buffer_freelist;
		bm->buffer_freelist = curr;
		
		curr = next;
	}
	
	bm->buffer_list = NULL;
}
