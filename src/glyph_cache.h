#ifndef GLYPH_CACHE
#define GLYPH_CACHE

/////////////////////////////////////////////////////////////////////
//	
//	~geb: glyph cache (OpenGL + stb_truetype)
//
/////////////////////////////////////////////////////////////////////

#include "base.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "thirdparty/stb/stb_truetype.h"

typedef struct {
	u64 value;
} Glyph_Hash;

typedef struct {
	u32 value; // packed Y<<16 | X
} GPU_Glyph_Index;

typedef struct {
	u32 x;
	u32 y;
} Glyph_Cache_Point;

force_inline GPU_Glyph_Index
glyph_pack_point(u32 x, u32 y)
{
	GPU_Glyph_Index r;
	r.value = (y << 16) | x;
	return r;
}

force_inline Glyph_Cache_Point
glyph_unpack_point(GPU_Glyph_Index p)
{
	Glyph_Cache_Point r;
	r.x = p.value & 0xffff;
	r.y = p.value >> 16;
	return r;
}

typedef struct {
	u32 id;
	GPU_Glyph_Index gpu_index;

	u32 filled;
	u16 dim_x;
	u16 dim_y;
} Glyph_State;

typedef struct {
	u32 hash_count; // pow of 2
	u32 entry_count;
	u32 reserved_tiles;
	u32 tiles_per_row;
} Glyph_Table_Params;

typedef struct Glyph_Table Glyph_Table;

typedef struct {
	Allocator alloc;
	Allocator scratch;

	u32 texture;

	i32 atlas_width;
	i32 atlas_height;
	i32 tile_width;
	i32 tile_height;

	i32 tiles_per_row;
	f32 scale;

	stbtt_fontinfo font;

	Glyph_Table *table;
	u8 *table_memory;
	usize table_memory_size;
} Glyph_Cache;


internal bool glyph_cache_make(
	Glyph_Cache *cache,
	u8 *ttf_data,
	f32 pixel_height,
	i32 atlas_w,
	i32 atlas_h,
	i32 tile_w,
	i32 tile_h,
	Glyph_Table_Params params,
	Allocator alloc, Allocator scratch
);

internal void glyph_cache_delete(Glyph_Cache *cache);
internal Glyph_State glyph_get(Glyph_Cache *cache, rune codepoint);

#endif
