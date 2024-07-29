#include <stdlib.h>

#include "chunk.h"
#include "memory.h"

/* init_chunk: initialize a chunk of bytecode. */
void init_chunk(Chunk *chunk)
{
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    chunk->line_runs = NULL;
    chunk->line_run_count = 0;
    chunk->line_run_capacity = 0;
    init_value_array(&chunk->constants);
}

/* free_chunk: delete a chunk of bytecode. */
void free_chunk(Chunk *chunk)
{
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    FREE_ARRAY(LineRun, chunk->line_runs, chunk->line_run_capacity);
    free_value_array(&chunk->constants);
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

    // Handle run-length encoding for line numbers
    if (chunk->line_run_count == 0 ||
            chunk->line_runs[chunk->line_run_count - 1].line != line) {
        if (chunk->line_run_capacity < chunk->line_run_count + 1) {
            int old_line_run_capacity = chunk->line_run_capacity;
            chunk->line_run_capacity = GROW_CAPACITY(old_line_run_capacity);
            chunk->line_runs = GROW_ARRAY(LineRun, chunk->line_runs,
                                          old_line_run_capacity, chunk->line_run_capacity);
        }
        chunk->line_runs[chunk->line_run_count].line = line;
        chunk->line_runs[chunk->line_run_count++].count = 1;
    } else {
        chunk->line_runs[chunk->line_run_count - 1].count++;
    }
}

/* add_constant: add a new constant to a chunk's constant pool. */
int add_constant(Chunk *chunk, Value value)
{
    write_value_array(&chunk->constants, value);
    return chunk->constants.count - 1;
}

/* int get_line: given a bytecode offset, determine the line where the instruction occurs. */
int get_line(Chunk *chunk, int offset)
{
    int accumulated_count = 0;
    for (int i = 0; i < chunk->line_run_count; i++) {
        accumulated_count += chunk->line_runs[i].count;
        if (offset < accumulated_count)
            return chunk->line_runs[i].line;
    }
    return -1;
}
