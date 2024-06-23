#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"

/* One-byte op-code structure. */
typedef enum {
    OP_RETURN,
} OpCode;

/* Dynamic array structure to hold sequences of bytecode. */
typedef struct {
    int count;      // number of allocated entries in use.
    int capacity;   // total number of allocated entires.
    uint8_t *code;  // an array of bytes.
} Chunk;

void init_chunk(Chunk *chunk);
void free_chunk(Chunk *chunk);
void write_chunk(Chunk *chunk, uint8_t byte);

#endif
