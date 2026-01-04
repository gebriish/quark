#include "base_arena.h"

internal Arena *
arena_new(
	usize reserve_size,
	MemReserveFn  reserve,
	MemCommitFn   commit,
	MemDecommitFn decommit,
	MemReleaseFn  release
) {
	if (!reserve || !commit || !release) {
		log_error("allocator callbacks must not be NULL");
		return NULL;
	}

	if (reserve_size > USIZE_MAX - sizeof(Arena)) {
		log_error("reserve size overflow (%zu)", reserve_size);
		return NULL;
	}

	usize total_size = sizeof(Arena) + reserve_size;

	void *base = reserve(total_size);
	if (!base) {
		log_error("reserve failed (size=%zu)", total_size);
		return NULL;
	}

	usize header_commit_size =
		AlignPow2(sizeof(Arena), COMMIT_BLOCK_SIZE);

	if (commit(base, header_commit_size)) {
		log_error("header commit failed (size=%zu)",
		         header_commit_size);
		release(base, total_size);
		return NULL;
	}

	Arena *arena = (Arena *)base;

	arena->base      = (u8 *)base + sizeof(Arena);
	arena->size      = reserve_size;
	arena->committed = header_commit_size > sizeof(Arena)
		? header_commit_size - sizeof(Arena)
		: 0;
	arena->pos       = 0;

	arena->reserve   = reserve;
	arena->commit    = commit;
	arena->decommit  = decommit;
	arena->release   = release;

	return arena;
}

internal void
arena_delete(Arena *arena) {
	if (!arena) {
		log_warn("NULL arena");
		return;
	}

	usize total_size = sizeof(Arena) + arena->size;

	arena->release((void *)arena, total_size);
}

internal void *
arena_alloc(
	Arena *arena,
	usize size,
	usize align,
	Source_Code_Location loc
) {
	(void)loc;

	if (!arena) {
		log_error("arena is NULL");
		return NULL;
	}

	if (size == 0) {
		log_warn("zero-size allocation ignored");
		return NULL;
	}

	usize aligned_pos = AlignPow2(arena->pos, align);
	usize new_pos     = aligned_pos + size;

	if (new_pos > arena->size) {
		log_error(
			"arena OOM: request=%zu used=%zu capacity=%zu",
			size, arena->pos, arena->size
		);
		return NULL;
	}

	if (new_pos > arena->committed) {
		usize needed      = new_pos - arena->committed;
		usize commit_size = AlignPow2(needed, COMMIT_BLOCK_SIZE);

		if (arena->committed + commit_size > arena->size) {
			commit_size = arena->size - arena->committed;
		}

		if (commit_size == 0) {
			log_error("commit_size=0 (corrupt state)");
			return NULL;
		}

		void *commit_ptr = arena->base + arena->committed;
		if (arena->commit(commit_ptr, commit_size) != 0) {
			log_error("commit failed (size=%zu)",
			         commit_size);
			return NULL;
		}

		arena->committed += commit_size;

		log_trace("+%zu bytes commited (total=%zu)",
		         commit_size, arena->committed);
	}

	arena->pos = new_pos;
	return arena->base + aligned_pos;
}

internal void *
arena_realloc(
	Arena *arena,
	void *ptr,
	usize new_size,
	usize old_size
) {
	if (!arena) {
		log_error("arena is NULL");
		return NULL;
	}

	if (!ptr) {
		log_warn("NULL ptr, realloc becomes alloc");
		return arena_alloc(arena, new_size, DEFAULT_ALIGNMENT, (Source_Code_Location){0});
	}

	if (new_size == 0) {
		log_warn("zero-size realloc ignored");
		return NULL;
	}

	u8 *expected_end = arena->base + arena->pos;
	u8 *actual_end   = (u8 *)ptr + old_size;

	bool is_last_alloc = (expected_end == actual_end);

	if (is_last_alloc) {
		usize offset     = (u8 *)ptr - (u8 *)arena->base;
		usize new_pos    = offset + new_size;

		if (new_pos > arena->size) {
			log_error("OOM: new_size=%zu", new_size);
			return NULL;
		}

		if (new_pos > arena->committed) {
			usize needed      = new_pos - arena->committed;
			usize commit_size = AlignPow2(needed, COMMIT_BLOCK_SIZE);

			if (arena->committed + commit_size > arena->size) {
				commit_size = arena->size - arena->committed;
			}

			if (commit_size == 0) {
				log_error("commit_size=0");
				return NULL;
			}

			void *commit_ptr = arena->base + arena->committed;
			if (arena->commit(commit_ptr, commit_size) != 0) {
				log_error("commit failed (size=%zu)",
				         commit_size);
				return NULL;
			}

			arena->committed += commit_size;

			log_trace("commit: +%zu bytes (total=%zu)",
			         commit_size, arena->committed);
		}

		arena->pos = new_pos;
		return ptr;
	}

	void *new_ptr = arena_alloc(
		arena,
		new_size,
		DEFAULT_ALIGNMENT,
		(Source_Code_Location){0}
	);

	if (!new_ptr) {
		log_error("fallback alloc failed");
		return NULL;
	}

	usize copy_size = old_size < new_size ? old_size : new_size;
	MemMove(new_ptr, ptr, copy_size);

	log_trace("moved %zu bytes", copy_size);
	return new_ptr;
}

internal void
arena_reset(Arena *arena) {
	Assert(arena);

	arena->pos = 0;
}

internal void
arena_clear(Arena *arena) {
	Assert(arena);

	if (arena->committed > 0 && arena->decommit) {
		log_trace("decommit %zu bytes",
		         arena->committed);

		arena->decommit(arena->base, arena->committed);
		arena->committed = 0;
	}

	arena->pos = 0;
}


internal void
arena_pop_to(Arena *arena, usize pos)
{
	Assert(arena && "Cannot pop to on a NULL arena");
	Assert(arena->pos >= pos && "Arena pop overshoots position");

	arena->pos = pos;
}

internal Arena_Scope
arena_scope_begin(Arena *arena)
{
	Assert(arena && "Scope cant begin on Null arena");

	Arena_Scope scope = {
		.arena = arena,
		.pos = arena->pos
	};
	return scope;
}

internal void
arena_scope_end(Arena_Scope scope)
{
	Assert(scope.arena && "Scope cant end on Null arena");
	Assert(scope.pos <= scope.arena->pos);

	scope.arena->pos = scope.pos;
}
