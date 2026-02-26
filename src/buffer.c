#include "buffer.h"

struct Q_Buffer {
    Allocator alloc;
    Q_Buffer *prev;
    Q_Buffer *next;

    usize gap_pos;
    usize gap_size;
    usize cap;

    u8 data[];
};

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
    usize size = sizeof(Q_Buffer) + new_cap;

    Alloc_Error err = 0;
    Q_Buffer *n = mem_alloc_aligned(b->alloc, size, AlignOf(Q_Buffer), false, &err);
    if (err) return b;

    MemZeroStruct(n);

    n->alloc = b->alloc;
    n->prev  = b->prev;
    n->next  = b->next;
    n->cap   = new_cap;

    usize before = b->gap_pos;
    usize after  = b->cap - (b->gap_pos + b->gap_size);

    n->gap_pos  = before;
    n->gap_size = new_cap - (before + after);

    MemMove(n->data, b->data, before);
    MemMove(n->data + n->gap_pos + n->gap_size,
            b->data + b->gap_pos + b->gap_size,
            after);

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
_current_column(Q_Buffer *b)
{
    usize start = _line_start(b, b->gap_pos);
    usize off = start;
    usize col = 0;

    while (off < b->gap_pos) {
        u8 c = b->data[_real(b, off)];
        u32 w = UTF8_LEN_TABLE[c];
        if (!w) w = 1;
        off += w;
        col++;
    }

    return col;
}

internal Q_Buffer *
buffer_new(String8 src, Q_Buffer *cur, Allocator alloc)
{
    usize cap = Max(src.len * 2, Kb(32));
    usize size = sizeof(Q_Buffer) + cap;

    Alloc_Error err = 0;
    Q_Buffer *b = mem_alloc_aligned(alloc, size, AlignOf(Q_Buffer), false, &err);
    if (err) return NULL;

    MemZeroStruct(b);

    b->alloc = alloc;
    b->cap   = cap;
    b->gap_size = cap - src.len;

    if (cur) {
        b->prev = cur;
        b->next = cur->next;
        if (b->next) b->next->prev = b;
        cur->next = b;
    }

    MemMove(b->data + b->gap_size, src.str, src.len);

    return b;
}

internal Q_Buffer *
buffer_insert(Q_Buffer *b, String8 text)
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
buffer_move_left(Q_Buffer *b)
{
    if (b->gap_pos == 0) return;

    usize pos = b->gap_pos - 1;

    while (pos && (b->data[_real(b, pos)] & 0xC0) == 0x80)
        pos--;

    _move_gap(b, pos);
}

internal void
buffer_move_right(Q_Buffer *b)
{
    usize len = _buf_len(b);
    if (b->gap_pos >= len) return;

    usize real = _real(b, b->gap_pos);
    u8 c = b->data[real];
    u32 w = UTF8_LEN_TABLE[c];
    if (!w) w = 1;

    _move_gap(b, b->gap_pos + w);
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

internal void
buffer_move_up(Q_Buffer *b)
{
    usize cur_start = _line_start(b, b->gap_pos);
    if (!cur_start) return;

    usize target_col = _current_column(b);

    usize prev_end   = cur_start - 1;
    usize prev_start = _line_start(b, prev_end);

    usize off = prev_start;
    usize col = 0;

    while (off < prev_end && col < target_col) {
        u8 c = b->data[_real(b, off)];
        u32 w = UTF8_LEN_TABLE[c];
        if (!w) w = 1;
        off += w;
        col++;
    }

    _move_gap(b, off);
}

internal void
buffer_move_down(Q_Buffer *b)
{
    usize len = _buf_len(b);
    usize next_start = _next_line_start(b, b->gap_pos);
    if (next_start >= len) return;

    usize target_col = _current_column(b);

    usize off = next_start;
    usize col = 0;

    while (off < len) {
        usize r = _real(b, off);
        if (b->data[r] == '\n') break;
        if (col >= target_col) break;

        u8 c = b->data[r];
        u32 w = UTF8_LEN_TABLE[c];
        if (!w) w = 1;
        off += w;
        col++;
    }

    _move_gap(b, off);
}
