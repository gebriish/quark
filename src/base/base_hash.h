#ifndef BASE_HASH_H
#define BASE_HASH_H

#include "base_core.h"

internal force_inline u32
hash32(u32 x)
{
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x);
    return x;
}

#define Map32_Define(name, value_t)                                                   \
typedef struct {                                                                      \
    u32 key;                                                                          \
    value_t value;                                                                    \
    bool occupied;                                                                    \
} name##_Entry;                                                                       \
                                                                                      \
typedef struct {                                                                      \
    name##_Entry *entries;                                                            \
    usize capacity;                                                                   \
    usize size;                                                                       \
} name;                                                                               \
                                                                                      \
internal name *                                                                       \
name##_new(Arena *arena, usize n)                                                     \
{                                                                                     \
    name *map = arena_push_struct(arena, name);                                       \
    map->size = 0;                                                                    \
    map->capacity = n * 2;                                                            \
    map->entries = arena_push_array_zeroed(arena, name##_Entry, map->capacity);       \
    return map;                                                                       \
}                                                                                     \
                                                                                      \
internal bool                                                                         \
name##_insert(name *map, u32 key, value_t value)                                      \
{                                                                                     \
    if (map->size >= map->capacity - 1) {                                             \
        return false;                                                                 \
    }                                                                                 \
    usize idx = hash32(key) % map->capacity;                                          \
    while (map->entries[idx].occupied) {                                              \
        if (map->entries[idx].key == key) {                                           \
            map->entries[idx].value = value;                                          \
            return true;                                                              \
        }                                                                             \
        idx = (idx + 1) % map->capacity;                                              \
    }                                                                                 \
    map->entries[idx].key = key;                                                      \
    map->entries[idx].value = value;                                                  \
    map->entries[idx].occupied = true;                                                \
    map->size += 1;                                                                   \
    return true;                                                                      \
}                                                                                     \
                                                                                      \
internal bool                                                                         \
name##_get(name *map, u32 key, value_t *out)                                          \
{                                                                                     \
    usize idx = hash32(key) % map->capacity;                                          \
    usize start = idx;                                                                \
    while (map->entries[idx].occupied) {                                              \
        if (map->entries[idx].key == key) {                                           \
            *out = map->entries[idx].value;                                           \
            return true;                                                              \
        }                                                                             \
        idx = (idx + 1) % map->capacity;                                              \
        if (idx == start) break;                                                      \
    }                                                                                 \
    return false;                                                                     \
}                                                                                     \
                                                                                      \
internal void                                                                         \
name##_clear(name *map)                                                               \
{                                                                                     \
    MemZero(map->entries, map->capacity * sizeof(name##_Entry));                      \
    map->size = 0;                                                                    \
}

#endif
