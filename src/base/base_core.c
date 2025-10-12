///////////////////////////////
// ~geb: Arena Allocator 

#include "base_core.h"

#include <stdlib.h>

internal Arena *
arena_alloc(usize capacity)
{
	void *base = malloc(capacity);
	Arena *arena = (Arena *) base;
	arena->previous = NULL;
	arena->current = arena;
	arena->last_used = ARENA_HEADER_SIZE;
	arena->used = ARENA_HEADER_SIZE;
	arena->capacity = capacity;
	return arena;
}

internal void
arena_release(Arena *arena)
{
	Assert(arena && "trying to release null arena");
	
	Arena *front = arena->current;
	
	while (front) {
		Arena *prev = front->previous;
		free(front);
		front = prev;
	}
}

internal void
arena_find_node_at_pos(Arena *arena, usize abs_pos, Arena **out_node, usize *out_local_pos)
{
	Arena *head = arena->current;

	usize chain_length = 0;
	Arena *node = head->previous;
	while (node) {
		chain_length++;
		node = node->previous;
	}

	usize base_offset = chain_length * head->capacity;

	if (abs_pos >= base_offset) {
		*out_node = head;
		*out_local_pos = abs_pos - base_offset;
	} else {
		node = head->previous;
		usize current_base = base_offset - head->capacity;

		while (node) {
			if (abs_pos >= current_base) {
				*out_node = node;
				*out_local_pos = abs_pos - current_base;
				return;
			}
			current_base -= node->capacity;
			node = node->previous;
		}

		AssertAlways(0 && "Invalid absolute position");
	}
}

internal void *
arena_push_(Arena *arena, Alloc_Params *params)
{
	Arena *head = arena->current;
	
	usize used_pre = AlignPow2(head->used, params->align);
	usize used_pst = used_pre + params->size;
	
	if (used_pst > head->capacity) {
		Arena *candidate = head->previous;
		while (candidate) {
			usize cand_pre = AlignPow2(candidate->used, params->align);
			usize cand_pst = cand_pre + params->size;
			
			if (cand_pst <= candidate->capacity) {
				candidate->last_used = candidate->used;
				void *result = (u8 *)candidate + cand_pre;
				candidate->used = cand_pst;
				
				if (params->zero) {
					MemZero(result, params->size);
				}
				
#if DEBUG_BUILD
				printf("arena_push %6zu bytes%s at %s:%d (%s) [reused chain node]\n",
					params->size,
					params->zero ? " [zero]" : "",
					params->caller_file,
					params->caller_line,
					params->caller_proc);
#endif
				return result;
			}
			candidate = candidate->previous;
		}
		
		usize new_capacity = head->capacity;
		Arena *new_arena = arena_alloc(new_capacity);
		new_arena->previous = head;
		
		// IMPORTANT(geb): Update ALL nodes' current pointer
		// Start from head (the actual front), not from arena parameter
		// because arena could be any node in the chain
		Arena *update = head;
		while (update) {
			update->current = new_arena;
			update = update->previous;
		}
		
		head = new_arena;
		used_pre = AlignPow2(head->used, params->align);
		used_pst = used_pre + params->size;
		
		AssertAlways(used_pst <= head->capacity && "Arena ran out of memory");
	}
	
	head->last_used = head->used;
	void *result = (u8 *)head + used_pre;
	head->used = used_pst;
	
	if (params->zero) {
		MemZero(result, params->size);
	}
	
#if DEBUG_BUILD
	printf("arena_push %6zu bytes%s at %s:%d (%s)\n",
		params->size,
		params->zero ? " [zero]" : "",
		params->caller_file,
		params->caller_line,
		params->caller_proc);
#endif
	
	return result;
}

internal usize
arena_pos(Arena *arena)
{
	Arena *head = arena->current;
	usize pos = head->used;

	Arena *node = head->previous;
	while (node) {
		pos += node->capacity;
		node = node->previous;
	}

	return pos;
}

internal void 
arena_pop(Arena *arena)
{
	Arena *head = arena->current;
	head->used = head->last_used;
}

internal void
arena_pop_to(Arena *arena, usize abs_pos)
{
	Arena *target_node;
	usize local_pos;
	arena_find_node_at_pos(arena, abs_pos, &target_node, &local_pos);
	
	Assert(local_pos >= ARENA_HEADER_SIZE && "Cannot pop into header");
	
	target_node->last_used = local_pos;
	target_node->used = local_pos;
	
	Arena *head = arena->current;
	if (target_node != head) {
		Arena *node = head;
		while (node != target_node) {
			node->used = ARENA_HEADER_SIZE;
			node->last_used = ARENA_HEADER_SIZE;
			node = node->previous;
		}
	}
}

internal void 
arena_clear(Arena *arena)
{
	Arena *node = arena->current;
	while (node) {
		node->used = ARENA_HEADER_SIZE;
		node->last_used = ARENA_HEADER_SIZE;
		node = node->previous;
	}
}

internal Temp 
temp_begin(Arena *arena)
{
	Temp result = {
		.arena = arena,
		.pos = arena_pos(arena)
	};
	return result;
}

internal void   
temp_end(Temp temp)
{
	arena_pop_to(temp.arena, temp.pos);
}

internal void
arena_print_usage(Arena *arena, const char *name)
{
#if DEBUG_BUILD
	if (!arena) {
		printf("Arena(%s): NULL\n", name ? name : "unnamed");
		return;
	}
	
	printf("Arena Chain(%s):\n", name ? name : "unnamed");
	
	usize total_used = 0;
	usize total_capacity = 0;
	int node_count = 0;
	usize abs_offset = 0;
	
	Arena *node = arena->current;
	while (node) {
		node_count++;
		node = node->previous;
	}
	
	Arena **nodes = (Arena **)malloc(sizeof(Arena *) * (usize) node_count);
	node = arena->current;
	for (int i = node_count - 1; i >= 0; i--) {
		nodes[i] = node;
		node = node->previous;
	}
	
	for (int i = 0; i < node_count; i++) {
		node = nodes[i];
		total_used += node->used;
		total_capacity += node->capacity;
		
		f64 used_pct = 0;
		if (node->capacity > 0) {
			used_pct = ((f64)node->used / (f64)node->capacity) * 100.0;
		}
		
		printf("  Node %d%s (abs offset: %zu):\n", i + 1, 
		       (node == arena->current) ? " [CURRENT]" : "",
		       abs_offset);
		printf("    used:      %zu bytes (abs: %zu)\n", node->used, abs_offset + node->used);
		printf("    last_used: %zu bytes\n", node->last_used);
		printf("    capacity:  %zu bytes\n", node->capacity);
		printf("    usage:     %.2f%%\n", used_pct);
		
		abs_offset += node->capacity;
	}
	
	free(nodes);
	
	f64 total_pct = 0;
	if (total_capacity > 0) {
		total_pct = ((f64)total_used / (f64)total_capacity) * 100.0;
	}
	
	printf("  Total:\n");
	printf("    nodes:          %d\n", node_count);
	printf("    used:           %zu bytes\n", total_used);
	printf("    capacity:       %zu bytes\n", total_capacity);
	printf("    usage:          %.2f%%\n", total_pct);
	printf("    absolute pos:   %zu bytes\n", arena_pos(arena));
#endif
}
