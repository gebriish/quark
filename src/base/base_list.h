#ifndef BASE_LIST_H
#define BASE_LIST_H

#include "base_core.h"

#define LinkedList_Define(name, value_t)   \
typedef struct name##_Node name##_Node;    \
struct name##_Node {                       \
	value_t name;                            \
	name##_Node *next;                       \
};                                         \
                                           \
typedef struct name##_List name##_List;    \
struct name##_List {                       \
	name##_Node *first;                      \
	name##_Node *last;                       \
	usize        length;                     \
};


#endif
