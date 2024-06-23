#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"

/* One-byte opcode structure. */
typedef enum {
    OP_CONSTANT,
    OP_RETURN,
} OpCode;

/* Dynamic array structure to hold sequences of bytecode. */
typedef struct {
    int count;              // number of allocated entries in use.
    int capacity;           // total number of allocated entires.
    uint8_t *code;          // an array of bytes.
    int *lines;             // line numbers, parellels the code array.
    ValueArray constants;   // constants associated w/ the chunk.
} Chunk;

void init_chunk(Chunk *chunk);
void free_chunk(Chunk *chunk);
void write_chunk(Chunk *chunk, uint8_t byte, int line);
int add_constant(Chunk *chunk, Value value);

#endif
