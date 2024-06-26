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

/* write_constant: adds 'value' to 'chunk''s const array,
   writes appropriate instruction to load the constant. */
void write_constant(Chunk *chunk, Value value, int line)
{
    int constant_index = add_constant(chunk, value);
    if (constant_index <= 0xFF) {                              // check if the constant's index fits within a single byte.
        write_chunk(chunk, OP_CONSTANT, line);
        write_chunk(chunk, (uint8_t)constant_index, line);
    } else {                                                   // if not, write the index as a 24-bit number.
        write_chunk(chunk, OP_CONSTANT_LONG, line);
        write_chunk(chunk, (uint8_t)(constant_index & 0xFF), line);         // low byte of the index.
        write_chunk(chunk, (uint8_t)(constant_index >> 8) & 0xFF, line);    // middle byte of the index.
        write_chunk(chunk, (uint8_t)(constant_index >> 16) & 0xFF, line);   // high byte of the index.
    }
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
