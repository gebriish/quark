#ifndef BASE_LIST_H
#define BASE_LIST_H

#include "base_core.h"
#include "base_arena.h"

#define Generate_List(T, Name)                                                    \
                                                                                  \
typedef struct Name Name;                                                         \
struct Name {                                                                     \
    T     *array;                                                                 \
    usize  len;                                                                   \
    usize  cap;                                                                   \
    Arena *arena;                                                                 \
};                                                                                \
                                                                                  \
internal Name                                                                     \
Name##_make(Arena *arena, usize initial_cap) {                                    \
    Name list = {0};                                                              \
    list.arena = arena;                                                           \
    list.cap   = initial_cap;                                                     \
    if (initial_cap) {                                                            \
        list.array = (T *)arena_alloc(                                            \
            arena,                                                                \
            sizeof(T) * initial_cap,                                              \
            AlignOf(T),                                                           \
            Code_Location);                                                       \
    }                                                                             \
    return list;                                                                  \
}                                                                                 \
                                                                                  \
internal void                                                                     \
Name##_reserve(Name *list, usize new_cap) {                                       \
    if (new_cap <= list->cap) return;                                             \
                                                                                  \
    usize old_size = sizeof(T) * list->cap;                                       \
    usize new_size = sizeof(T) * new_cap;                                         \
                                                                                  \
    list->array = (T *)arena_realloc(                                             \
        list->arena,                                                              \
        list->array,                                                              \
        new_size,                                                                 \
        old_size);                                                                \
                                                                                  \
    list->cap = new_cap;                                                          \
}                                                                                 \
                                                                                  \
internal void                                                                     \
Name##_push(Name *list, T value) {                                                \
    if (list->len + 1 > list->cap) {                                              \
        usize new_cap = list->cap ? list->cap * 2 : 8;                            \
        Name##_reserve(list, new_cap);                                            \
    }                                                                             \
    list->array[list->len++] = value;                                             \
}                                                                                 \
                                                                                  \
force_inline T *                                                                  \
Name##_at(Name *list, usize index) {                                              \
    Assert(index < list->len);                                                    \
    return &list->array[index];                                                   \
}                                                                                 \
                                                                                  \
force_inline T *                                                                  \
Name##_last(Name *list) {                                                         \
    Assert(list->len > 0);                                                        \
    return &list->array[list->len - 1];                                           \
}                                                                                 \
                                                                                  \
force_inline void                                                                 \
Name##_clear(Name *list) {                                                        \
    list->len = 0;                                                                \
}


internal force_inline u32
hash32(u32 x)
{
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x);
    return x;
}

#define Generate_Map32(name, value_t)                                   \
                                                                        \
typedef struct {                                                        \
    u32     key;                                                        \
    value_t value;                                                      \
    bool    used;                                                       \
} name##_Entry;                                                         \
                                                                        \
typedef struct {                                                        \
    name##_Entry *entries;                                              \
    usize         cap;                                                  \
    usize         len;                                                  \
    Arena        *arena;                                                \
} name;                                                                 \
                                                                        \
internal name                                                           \
name##_make(Arena *arena, usize cap)                                    \
{                                                                       \
    if (cap < 8) cap = 8;                                               \
                                                                        \
    name m = {0};                                                       \
    m.arena   = arena;                                                  \
    m.cap     = cap;                                                    \
    m.entries = arena_alloc(arena,                                      \
                            sizeof(name##_Entry) * cap,                 \
                            AlignOf(name##_Entry),                      \
                            Code_Location);                             \
    MemZero(m.entries, sizeof(name##_Entry) * cap);                     \
    return m;                                                           \
}                                                                       \
                                                                        \
internal bool                                                           \
name##_put(name *m, u32 key, value_t value)                             \
{                                                                       \
    if (m->cap == 0) return false;                                      \
                                                                        \
    usize i     = hash32(key) % m->cap;                                 \
    usize start = i;                                                    \
                                                                        \
    for (;;)                                                            \
    {                                                                   \
        name##_Entry *e = &m->entries[i];                               \
                                                                        \
        if (!e->used)                                                   \
        {                                                               \
            e->used  = true;                                            \
            e->key   = key;                                             \
            e->value = value;                                           \
            m->len++;                                                   \
            return true;                                                \
        }                                                               \
                                                                        \
        if (e->key == key)                                              \
        {                                                               \
            e->value = value;                                           \
            return true;                                                \
        }                                                               \
                                                                        \
        i = (i + 1) % m->cap;                                           \
        if (i == start) break; /* table full */                         \
    }                                                                   \
                                                                        \
    return false;                                                       \
}                                                                       \
                                                                        \
internal bool                                                           \
name##_get(name *m, u32 key, value_t *out)                              \
{                                                                       \
    if (m->cap == 0) return false;                                      \
                                                                        \
    usize i     = hash32(key) % m->cap;                                 \
    usize start = i;                                                    \
                                                                        \
    for (;;)                                                            \
    {                                                                   \
        name##_Entry *e = &m->entries[i];                               \
                                                                        \
        if (!e->used) return false; /* probe chain ended */             \
                                                                        \
        if (e->key == key)                                              \
        {                                                               \
            *out = e->value;                                            \
            return true;                                                \
        }                                                               \
                                                                        \
        i = (i + 1) % m->cap;                                           \
        if (i == start) break;                                          \
    }                                                                   \
                                                                        \
    return false;                                                       \
}                                                                       \
                                                                        \
internal void                                                           \
name##_clear(name *m)                                                   \
{                                                                       \
    MemZero(m->entries, sizeof(name##_Entry) * m->cap);                 \
    m->len = 0;                                                         \
}

typedef u32 Queue_Error;
enum {
	Queue_Err_None,
	Queue_Err_Overflow,
	Queue_Err_Underflow,
};

#define Generate_Deque(Name, T)                                              \
                                                                             \
typedef struct Name Name;                                                    \
struct Name {                                                                \
    T     *data;                                                             \
    usize  capacity;                                                         \
    usize  front;    /* index of first element */                            \
    usize  back;     /* one past last element */                             \
    usize  len;                                                              \
};                                                                           \
                                                                             \
internal Name                                                                \
Name##_make(Arena *arena, usize max_size)                                    \
{                                                                            \
    Assert(arena);                                                           \
                                                                             \
    Name dq = {0};                                                           \
                                                                             \
    dq.data     = arena_push_array(arena, T, max_size);                      \
    dq.capacity = max_size;                                                  \
    dq.front    = 0;                                                         \
    dq.back     = 0;                                                         \
    dq.len      = 0;                                                         \
                                                                             \
    return dq;                                                               \
}                                                                            \
                                                                             \
internal void                                                                \
Name##_clear(Name *dq)                                                       \
{                                                                            \
    Assert(dq);                                                              \
                                                                             \
    dq->front = 0;                                                           \
    dq->back  = 0;                                                           \
    dq->len   = 0;                                                           \
}                                                                            \
                                                                             \
internal Queue_Error                                                         \
Name##_push_back(Name *dq, T val)                                            \
{                                                                            \
    Assert(dq);                                                              \
                                                                             \
    if (dq->len >= dq->capacity)                                             \
        return Queue_Err_Overflow;                                           \
                                                                             \
    dq->data[dq->back] = val;                                                \
    dq->back = (dq->back + 1) % dq->capacity;                                \
    dq->len++;                                                               \
                                                                             \
    return Queue_Err_None;                                                   \
}                                                                            \
                                                                             \
internal Queue_Error                                                         \
Name##_push_front(Name *dq, T val)                                           \
{                                                                            \
    Assert(dq);                                                              \
                                                                             \
    if (dq->len >= dq->capacity)                                             \
        return Queue_Err_Overflow;                                           \
                                                                             \
    dq->front = (dq->front + dq->capacity - 1) % dq->capacity;               \
    dq->data[dq->front] = val;                                               \
    dq->len++;                                                               \
                                                                             \
    return Queue_Err_None;                                                   \
}                                                                            \
                                                                             \
internal Queue_Error                                                         \
Name##_pop_front(Name *dq, T *out)                                           \
{                                                                            \
    Assert(dq);                                                              \
    Assert(out);                                                             \
                                                                             \
    if (dq->len == 0)                                                        \
        return Queue_Err_Underflow;                                          \
                                                                             \
    *out = dq->data[dq->front];                                              \
    dq->front = (dq->front + 1) % dq->capacity;                              \
    dq->len--;                                                               \
                                                                             \
    return Queue_Err_None;                                                   \
}                                                                            \
                                                                             \
internal Queue_Error                                                         \
Name##_pop_back(Name *dq, T *out)                                            \
{                                                                            \
    Assert(dq);                                                              \
    Assert(out);                                                             \
                                                                             \
    if (dq->len == 0)                                                        \
        return Queue_Err_Underflow;                                          \
                                                                             \
    dq->back = (dq->back + dq->capacity - 1) % dq->capacity;                 \
    *out = dq->data[dq->back];                                               \
    dq->len--;                                                               \
                                                                             \
    return Queue_Err_None;                                                   \
}

#endif
