#include "buffer.h"

// const global usize TAB_WIDTH = 4;

internal usize
_buf_len(Q_Buffer *b)
{
    return b->cap - b->gap_size;
}

internal usize
_real(Q_Buffer *b, usize off)
{
    return off < b->gap_pos ? off : off + b->gap_size;
}

internal void
_move_gap(Q_Buffer *b, usize off)
{
    if (off == b->gap_pos) return;

    if (off < b->gap_pos) {
        usize n = b->gap_pos - off;
        MemMove(b->data + off + b->gap_size,
                b->data + off,
                n);
    } else {
        usize n = off - b->gap_pos;
        MemMove(b->data + b->gap_pos,
                b->data + b->gap_pos + b->gap_size,
                n);
    }

    b->gap_pos = off;
}

internal Q_Buffer *
_grow(Q_Buffer *b, usize need)
{
    if (b->gap_size >= need) return b;

    usize new_cap = b->cap * 2 + need;

    usize name_len = b->name.len;
    usize size = sizeof(Q_Buffer) + new_cap + name_len;

    Alloc_Error err = 0;
    Q_Buffer *n = mem_alloc_aligned(b->alloc, size, AlignOf(Q_Buffer), false, &err);
    if (err) return b;

    MemZeroStruct(n);

    n->alloc = b->alloc;
    n->prev  = b->prev;
    n->next  = b->next;
    n->cap   = new_cap;
	n->goal_col = b->goal_col;
	n->goal_col_valid = b->goal_col_valid;

    u8 *name_mem = n->data + new_cap;
    MemMove(name_mem, b->name.str, name_len);
    n->name = str8(name_mem, name_len);

    usize before = b->gap_pos;
    usize after  = b->cap - (b->gap_pos + b->gap_size);

    n->gap_pos  = before;
    n->gap_size = new_cap - (before + after);

    MemMove(n->data, b->data, before);
    MemMove(n->data + n->gap_pos + n->gap_size, b->data + b->gap_pos + b->gap_size, after);

    if (n->prev) n->prev->next = n;
    if (n->next) n->next->prev = n;

    mem_free(b->alloc, b, NULL);
    return n;
}

internal usize
_line_start(Q_Buffer *b, usize off)
{
    while (off) {
        usize r = _real(b, off - 1);
        if (b->data[r] == '\n') break;
        off--;
    }
    return off;
}

internal usize
_next_line_start(Q_Buffer *b, usize off)
{
    usize len = _buf_len(b);
    while (off < len) {
        usize r = _real(b, off);
        if (b->data[r] == '\n') return off + 1;
        off++;
    }
    return len;
}

internal usize
_current_column(Q_Buffer *b, int tab_width)
{
    usize start = _line_start(b, b->gap_pos);
    usize off = start;
    usize col = 0;

    while (off < b->gap_pos) {
        usize r = _real(b, off);
        u8 c = b->data[r];

        u32 w = UTF8_LEN_TABLE[c];
        if (!w) w = 1;

        off += w;

        if (c == '\t') {
            usize advance = (usize) (tab_width - (col % tab_width));
            col += advance;
        } else {
            col++;
        }
    }

    return col;
}

internal Q_Buffer *
buffer_make(String8 name, String8 src, Q_Buffer *cur, Allocator alloc)
{
    usize buffer_cap = Max(src.len * 2, Kb(4));
    usize size = sizeof(Q_Buffer) + buffer_cap + name.len;

    Alloc_Error err = 0;
    Q_Buffer *b = cast(Q_Buffer *) mem_alloc_aligned(alloc, size, AlignOf(Q_Buffer), false, &err);
    if (err) return NULL;

    MemZeroStruct(b);

    b->alloc = alloc;
    b->cap   = buffer_cap;
    b->gap_size = buffer_cap - src.len;
	b->name = str8(cast(u8 *) b + sizeof(Q_Buffer) + buffer_cap, name.len);
	MemMove(b->name.str, name.str, name.len);

    if (cur) {
        b->prev = cur;
        b->next = cur->next;
        if (b->next) b->next->prev = b;
        cur->next = b;
    }

    MemMove(b->data + b->gap_size, src.str, src.len);

    return b;
}

internal void
buffer_delete(Q_Buffer *b)
{
	if (!b) return;

	if (b->prev) b->prev->next = b->next;
	if (b->next) b->next->prev = b->prev;

	mem_free(b->alloc, b, NULL);
}

internal Q_Buffer *
buffer_insert(Q_Buffer *b, String8 text, int tab_width)
{
    b = _grow(b, text.len);

    MemMove(b->data + b->gap_pos, text.str, text.len);

    for (usize i = 0; i < text.len;) {
        u8 c = text.str[i];
        u32 w = UTF8_LEN_TABLE[c];
        if (!w) w = 1;

        i += w;
    }

    b->gap_pos  += text.len;
    b->gap_size -= text.len;

    b->goal_col = _current_column(b, tab_width);
    b->goal_col_valid = true;

    return b;
}

internal void
buffer_erase(Q_Buffer *b)
{
    usize len = _buf_len(b);
    if (b->gap_pos >= len) return;

    usize real = _real(b, b->gap_pos);
    u8 c = b->data[real];
    u32 w = UTF8_LEN_TABLE[c];
    if (!w) w = 1;

    b->gap_size += w;
}

internal void
buffer_move_left(Q_Buffer *b, int tab_width)
{
    if (b->gap_pos == 0) return;

    usize pos = b->gap_pos - 1;

    while (pos && (b->data[_real(b, pos)] & 0xC0) == 0x80)
        pos--;

    _move_gap(b, pos);

    b->goal_col = _current_column(b, tab_width);
    b->goal_col_valid = true;
}

internal void
buffer_move_right(Q_Buffer *b, int tab_width)
{
    usize len = _buf_len(b);
    if (b->gap_pos >= len) return;

    usize real = _real(b, b->gap_pos);
    u8 c = b->data[real];
    u32 w = UTF8_LEN_TABLE[c];
    if (!w) w = 1;

    _move_gap(b, b->gap_pos + w);

    b->goal_col = _current_column(b, tab_width);
    b->goal_col_valid = true;
}

internal void
buffer_move_up(Q_Buffer *b, int tab_width)
{
    usize cur_start = _line_start(b, b->gap_pos);
    if (cur_start == 0) return;

    if (!b->goal_col_valid) {
        b->goal_col = _current_column(b, tab_width);
        b->goal_col_valid = true;
    }

    usize prev_end   = cur_start - 1;
    usize prev_start = _line_start(b, prev_end);

    usize off = prev_start;
    usize col = 0;

    while (off < prev_end && col < b->goal_col) {
        u8 c = b->data[_real(b, off)];
        u32 w = UTF8_LEN_TABLE[c];
        if (!w) w = 1;

		if (b->data[_real(b, off)] == '\t')
			col += 4;
		else
			col++;
		off += w;
    }

    _move_gap(b, off);
}

internal void
buffer_move_down(Q_Buffer *b, int tab_width)
{
	usize len = _buf_len(b);

	usize next_start = _next_line_start(b, b->gap_pos);
	if (next_start >= len) return;

	if (!b->goal_col_valid) {
		b->goal_col = _current_column(b, tab_width);
		b->goal_col_valid = true;
	}

	usize off = next_start;
	usize col = 0;

	while (off < len) {
		usize r = _real(b, off);
		if (b->data[r] == '\n') break;
		if (col >= b->goal_col) break;

		u8 c = b->data[r];
		u32 w = UTF8_LEN_TABLE[c];
		if (!w) w = 1;

		off += w;

		if (b->data[r] == '\t')
			col += 4;
		else
			col++;
	}

	_move_gap(b, off);
}


internal usize
buffer_current_indent_depth(Q_Buffer *buffer, int tab_width)
{
	usize start = _line_start(buffer, buffer->gap_pos);
	usize off = start;
	usize col = 0;

	while (off < buffer->gap_pos) {
		usize r = _real(buffer, off);
		u8 c = buffer->data[r];

		if (c == ' ') {
			col += 1;
			off += 1;
		}
		else if (c == '\t') {
			usize advance = tab_width - (col % tab_width);
			col += advance;
			off += 1;
		}
		else {
			break;
		}
	}

	return col / tab_width;
}


internal rune
buffer_peek(Q_Buffer *buffer)
{
	if (!buffer) return 0;

	usize pos = buffer->gap_pos + buffer->gap_size;
	usize len = buffer->cap;

	if (pos >= len) return 0;

	UTF8_Error err = 0;
	rune cp = utf8_decode(buffer->data + pos, &err);

	if (err) return 0;

	return cp;
}

internal rune
buffer_peek_prev(Q_Buffer *buffer)
{
	return 0;
}

internal bool
buffer_iter(Q_Buffer *b, Q_Iterator *it)
{
    usize len = _buf_len(b);
    if (it->offset >= len) return false;

    usize real = _real(b, it->offset);
    u8 *ptr = b->data + real;

    UTF8_Error err;
    it->codepoint = utf8_decode(ptr, &err);
    it->is_on_cursor = (it->offset == b->gap_pos);

    u32 w = UTF8_LEN_TABLE[*ptr];
    if (!w) w = 1;
    it->offset += w;

    if (it->codepoint == '\n') {
        it->position.row++;
        it->position.col = 0;
    } else {
        it->position.col++;
    }

    return true;
}


internal String8
buffer_slice(Q_Buffer *buffer, usize begin, usize end, Allocator alloc)
{
    Assert(begin <= end);
    Assert(end <= _buf_len(buffer));

    usize begin_real = _real(buffer, begin);
    usize end_real   = _real(buffer, end);

    usize gap_begin = buffer->gap_pos;
    usize gap_end   = buffer->gap_pos + buffer->gap_size;

    usize size = end - begin;

    if (end_real <= gap_begin)
    {
        return (String8){
            .str  = buffer->data + begin_real,
            .len = size
        };
    }

    if (begin_real >= gap_end)
    {
        return (String8){
            .str  = buffer->data + begin_real,
            .len = size
        };
    }

    u8 *dst = alloc_array_nz(alloc, u8, size, NULL);

    usize left_size  = gap_begin - begin_real;
    usize right_size = end_real - gap_end;

	MemMove(dst,
		 buffer->data + begin_real,
		 left_size);

	MemMove(dst + left_size,
		 buffer->data + gap_end,
		 right_size);

    return (String8){
        .str  = dst,
        .len = size
    };
}
