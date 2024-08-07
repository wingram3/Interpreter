#ifndef clox_memory_h
#define clox_memory_h

#include "common.h"
#include "object.h"

/* ALLOCATE MACRO: allocates an array with a given element type and count. */
#define ALLOCATE(type, count) \
    (type *)reallocate(NULL, 0, sizeof(type) * (count))

/* GROW_CAPACITY MACRO: calculates new capacity based on current capacity. */
#define GROW_CAPACITY(capacity) \
    ((capacity) < 8 ? 8 : (capacity) * 2)

/* GROW_ARRAY MACRO: Grow a chunk's array to the desired capacity. */
#define GROW_ARRAY(type, pointer, old_count, new_count) \
    (type *)reallocate(pointer, sizeof(type) * (old_count), \
        sizeof(type) * (new_count))

/* FREE_ARRAY MACRO: Free a chunk's memory by passing in zero for new size. */
#define FREE_ARRAY(type, pointer, old_count) \
    reallocate(pointer, sizeof(type) * (old_count), 0)

#define FREE(type, pointer) reallocate(pointer, sizeof(type), 0)

void *reallocate(void *pointer, size_t old_size, size_t new_size);
void free_objects();

#endif
