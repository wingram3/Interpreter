#include <stdlib.h>

#include "chunk.h"
#include "lines.h"
#include "memory.h"

/* init_chunk: initialize a chunk of bytecode. */
void init_chunk(Chunk *chunk)
{
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    init_value_array(&chunk->constants);
    init_line_number_array(&chunk->lines);
}

/* free_chunk: delete a chunk of bytecode. */
void free_chunk(Chunk *chunk)
{
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    free_value_array(&chunk->constants);
    free_line_number_array(&chunk->lines);
    init_chunk(chunk);
}

/* write_chunk: append a byte to the end of a chunk. */
void write_chunk(Chunk *chunk, uint8_t byte, int line)
{
    if (chunk->capacity < chunk->count + 1) {
        int old_capacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(old_capacity);
        chunk->code = GROW_ARRAY(uint8_t, chunk->code,
            old_capacity, chunk->capacity);
    }

    chunk->code[chunk->count] = byte;
    chunk->count++;
    add_line(chunk, line);
}

/* add_constant: add a new constant to a chunk's constant pool. */
int add_constant(Chunk *chunk, Value value)
{
    write_value_array(&chunk->constants, value);
    return chunk->constants.count - 1;
}

/* add_line: adds a LineNumberEntry to the chunk's line number array
   if the thing being written is on a new line. */
void add_line(Chunk *chunk, int line)
{
    if (chunk->lines.count == 0 || chunk->lines.line_number_entries[chunk->lines.count - 1].line_number != line) {
        LineNumberEntry entry;
        entry.bytecode_offset = chunk->count - 1;
        entry.line_number = line;
        write_line_number_array(&chunk->lines, entry);
    } else return;
}
