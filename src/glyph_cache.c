#include "glyph_cache.h"

#include "gfx.h"

/////////////////////////////////////////////
// ~geb: internal glyph table

typedef struct {
    Glyph_Hash hash;

    u32 next_hash;
    u32 prev_lru;
    u32 next_lru;

    GPU_Glyph_Index gpu_index;

    u32 filled;

    u16 dim_x;
    u16 dim_y;

    f32 advance;
    f32 bearing_x;
    f32 bearing_y;
} Glyph_Entry;

struct Glyph_Table {
	u32 hash_mask;
	u32 hash_count;
	u32 entry_count;

	u32 *hash_table;
	Glyph_Entry *entries;

	u32 lru_head;
	u32 lru_tail;

	u32 free_list;
};

internal bool
_is_pow2(u32 v)
{
	return v && !(v & (v - 1));
}

internal usize
_glyph_table_footprint(Glyph_Table_Params p)
{
	return sizeof(Glyph_Table) + p.hash_count * sizeof(u32) + p.entry_count * sizeof(Glyph_Entry);
}

internal Glyph_Table *
_glyph_table_place(Glyph_Table_Params p, void *memory)
{
	if (!memory) return 0;

	assert(_is_pow2(p.hash_count));

	Glyph_Table *t = memory;
	MemZero(t, sizeof(*t));

	u8 *base = (u8*)memory + sizeof(Glyph_Table);

	t->hash_table = (u32*)base;
	t->entries    = (Glyph_Entry*)(base + p.hash_count * sizeof(u32));

	t->hash_mask   = p.hash_count - 1;
	t->hash_count  = p.hash_count;
	t->entry_count = p.entry_count;

	MemZero(t->hash_table, p.hash_count * sizeof(u32));

	for (u32 i = 1; i < p.entry_count; ++i)
		t->entries[i].next_hash = i + 1;

	t->entries[p.entry_count - 1].next_hash = 0;
	t->free_list = 1;

	u32 x = p.reserved_tiles % p.tiles_per_row;
	u32 y = p.reserved_tiles / p.tiles_per_row;

	for (u32 i = 0; i < p.entry_count; ++i) {
		t->entries[i].gpu_index = glyph_pack_point(x, y);

		x++;
		if (x >= p.tiles_per_row) {
			x = 0;
			y++;
		}
	}

	return t;
}

internal void
_lru_remove(Glyph_Table *t, u32 id)
{
	Glyph_Entry *e = &t->entries[id];

	if (e->prev_lru) t->entries[e->prev_lru].next_lru = e->next_lru;
	if (e->next_lru) t->entries[e->next_lru].prev_lru = e->prev_lru;

	if (t->lru_head == id) t->lru_head = e->next_lru;
	if (t->lru_tail == id) t->lru_tail = e->prev_lru;

	e->prev_lru = e->next_lru = 0;
}

internal void
_lru_insert_front(Glyph_Table *t, u32 id)
{
	Glyph_Entry *e = &t->entries[id];

	e->prev_lru = 0;
	e->next_lru = t->lru_head;

	if (t->lru_head)
		t->entries[t->lru_head].prev_lru = id;

	t->lru_head = id;

	if (!t->lru_tail)
		t->lru_tail = id;
}

internal u32
_alloc_entry(Glyph_Table *t)
{
	if (t->free_list) {
		u32 id = t->free_list;
		t->free_list = t->entries[id].next_hash;
		return id;
	}

	u32 id = t->lru_tail;
	_lru_remove(t, id);

	u32 slot = t->entries[id].hash.value & t->hash_mask;
	u32 *cur = &t->hash_table[slot];

	while (*cur && *cur != id)
		cur = &t->entries[*cur].next_hash;

	if (*cur)
		*cur = t->entries[id].next_hash;

	t->entries[id].filled = 0;
	t->entries[id].dim_x = 0;
	t->entries[id].dim_y = 0;

	return id;
}

internal Glyph_State
_table_find(Glyph_Table *t, Glyph_Hash hash)
{
	u32 slot = hash.value & t->hash_mask;
	u32 id = t->hash_table[slot];

	while (id) {
		if (t->entries[id].hash.value == hash.value) {
			_lru_remove(t, id);
			_lru_insert_front(t, id);

			Glyph_Entry *e = &t->entries[id];
			return (Glyph_State){
				id,
				e->gpu_index,
				e->filled,
				e->dim_x,
				e->dim_y
			};
		}
		id = t->entries[id].next_hash;
	}

	id = _alloc_entry(t);

	Glyph_Entry *e = &t->entries[id];
	e->hash = hash;

	e->next_hash = t->hash_table[slot];
	t->hash_table[slot] = id;

	_lru_insert_front(t, id);

	return (Glyph_State){
		id,
		e->gpu_index,
		0,
		0,
		0
	};
}

/////////////////////////////////////////////
// ~geb: public API

internal bool
glyph_cache_make(
	Glyph_Cache *cache,
	u8 *ttf_data,
	f32 pixel_height,
	i32 atlas_w,
	i32 atlas_h,
	i32 tile_w,
	i32 tile_h,
	Glyph_Table_Params params,
	Allocator alloc, Allocator scratch
) {
	MemZeroStruct(cache);

	if (!stbtt_InitFont(&cache->font, ttf_data, 0))
		return false;

	cache->alloc = alloc;
	cache->scratch = scratch;
	cache->scale = stbtt_ScaleForPixelHeight(&cache->font, pixel_height);

	cache->atlas_width  = atlas_w;
	cache->atlas_height = atlas_h;
	cache->tile_width   = tile_w;
	cache->tile_height  = tile_h;
	cache->tiles_per_row = atlas_w / tile_w;

	params.tiles_per_row = cache->tiles_per_row;

	Image atlas_img = {
		.width = atlas_w,
		.height = atlas_h,
		.pixel_fmt = Pixel_R8,
		.data = 0
	};

	cache->texture = gfx_texture_upload(atlas_img, TextureKind_GreyScale);

	cache->table_memory_size = _glyph_table_footprint(params);

	Alloc_Error err = 0;
	cache->table_memory = alloc_array(alloc, u8, cache->table_memory_size, &err);
	if (err) {
		log_error("Failed to allocate glyph table memory, error(%d)", err); Trap();
	}

	cache->table = _glyph_table_place(params, cache->table_memory);

	return cache->table != 0;
}

internal void
glyph_cache_delete(Glyph_Cache *cache)
{
	if (cache->texture) {
		gfx_texture_unload(cache->texture);
		cache->texture = 0;
	}

	if (cache->table_memory)
		mem_free(cache->alloc, cache->table_memory, NULL);

	MemZeroStruct(cache);
}

internal Glyph_State
glyph_get(Glyph_Cache *cache, rune codepoint)
{
    Arena_Scope scratch = arena_scope_begin(cache->scratch.data);

    Glyph_Hash  hash  = { codepoint };
    Glyph_State state = _table_find(cache->table, hash);

    if (!state.filled)
    {
        int glyph = stbtt_FindGlyphIndex(&cache->font, codepoint);
        if (glyph == 0)
            glyph = stbtt_FindGlyphIndex(&cache->font, '?');

        int ascent, descent, line_gap;
        stbtt_GetFontVMetrics(&cache->font, &ascent, &descent, &line_gap);

        f32 scale = cache->scale;

        int x0, y0, x1, y1;
        stbtt_GetGlyphBitmapBox(&cache->font,
                                glyph,
                                scale, scale,
                                &x0, &y0, &x1, &y1);

        int w = x1 - x0;
        int h = y1 - y0;

        Glyph_Entry *e = &cache->table->entries[state.id];

        int advance, lsb;
        stbtt_GetGlyphHMetrics(&cache->font, glyph, &advance, &lsb);

        Glyph_Cache_Point p = glyph_unpack_point(state.gpu_index);

        int tile_x = p.x * cache->tile_width;
        int tile_y = p.y * cache->tile_height;

        if (w > 0 && h > 0)
        {
            u8 *bitmap = alloc_array(cache->scratch, u8, w*h, 0);

            stbtt_MakeGlyphBitmap(&cache->font,
                                  bitmap,
                                  w, h, w,
                                  scale, scale,
                                  glyph);

            int baseline = (int)(ascent * scale);

            int dst_x = tile_x + x0;
            int dst_y = tile_y + baseline + y0;

            Image img = { w, h, Pixel_R8, bitmap };
            gfx_texture_sub_data(cache->texture, dst_x, dst_y, img);
        }

        e->advance   = (f32)advance * scale;
        e->bearing_x = 0; // not needed anymore for terminal cell rendering
        e->bearing_y = 0;
        e->filled    = 1;

        state.filled = 1;
    }

    arena_scope_end(scratch);
    return state;
}
