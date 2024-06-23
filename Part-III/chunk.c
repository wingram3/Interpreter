#include <stdlib.h>

#include "chunk.h"
#include "memory.h"

/* init_chunk: initialize a chunk of bytecode. */
void init_chunk(Chunk *chunk)
{
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    init_value_array(&chunk->constants);
}

/* free_chunk: delete a chunk of bytecode. */
void free_chunk(Chunk *chunk)
{
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    free_value_array(&chunk->constants);
    init_chunk(chunk);
}

/* write_chunk: append a byte (opcode instruction) to the end of a chunk. */
void write_chunk(Chunk *chunk, uint8_t byte)
{
    if (chunk->capacity < chunk->count + 1) {
        int old_capacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(old_capacity);
        chunk->code = GROW_ARRAY(uint8_t, chunk->code,
            old_capacity, chunk->capacity);
    }

    chunk->code[chunk->count] = byte;
    chunk->count++;
}

/* add_constant: add a new constant to a chunk's value array. */
int add_constant(Chunk *chunk, Value value)
{
    write_value_array(&chunk->constants, value);
    return chunk->constants.count - 1;
}
