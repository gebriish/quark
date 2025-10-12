#ifndef QUARK_BUFFER_H
#define QUARK_BUFFER_H

#include "../base/base_core.h"
#include "../base/base_string.h"


typedef struct Gap_Buffer Gap_Buffer;
struct Gap_Buffer {
	usize capacity;
	usize gap_begin;
	usize gap_length;
	u8 *buffer;
};

typedef struct Buffer_Manager Buffer_Manager;
struct Buffer_Manager {
	Arena *allocator;
};


#endif
