#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"

/* One-byte opcode enum. */
typedef enum {
    OP_CONSTANT,
    OP_CONSTANT_LONG,
    OP_ZERO,
    OP_ONE,
    OP_TWO,
    OP_NIL,
    OP_TRUE,
    OP_FALSE,
    OP_POP,
    OP_POPN,
    OP_GET_GLOBAL,
    OP_GET_GLOBAL_LONG,
    OP_SET_GLOBAL,
    OP_SET_GLOBAL_LONG,
    OP_SET_LOCAL,
    OP_GET_LOCAL,
    OP_DEFINE_GLOBAL,
    OP_DEFINE_GLOBAL_LONG,
    OP_EQUAL,
    OP_GREATER,
    OP_GREATER_EQUAL,
    OP_LESS,
    OP_LESS_EQUAL,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NOT,
    OP_NOT_EQUAL,
    OP_NEGATE,
    OP_LOOP,
    OP_JUMP,
    OP_JUMP_IF_TRUE,
    OP_JUMP_IF_FALSE,
    OP_JUMP_NOT_EQUAL,
    OP_PRINT,
    OP_CALL,
    OP_RETURN,
} OpCode;

/* Line run compression - count of instructions on a corresponding line number. */
typedef struct {
    int line;   // Line number.
    int count;  // Number of consecutive instructions on this line.
} LineRun;

/* Dynamic array structure to hold sequences of bytecode. */
typedef struct {
    uint8_t *code;          // an array of bytes.
    int count;              // number of allocated entries in use.
    int capacity;           // total number of allocated entires.

    LineRun *line_runs;     // array of LineRun structs for RLE of line numbers.
    int line_run_count;     // number of LineRun entries.
    int line_run_capacity;  // total capacity of the line runs array.

    ValueArray constants;   // constants associated w/ the chunk.
} Chunk;

void init_chunk(Chunk *chunk);
void free_chunk(Chunk *chunk);
void write_chunk(Chunk *chunk, uint8_t byte, int line);
void write_constant(Chunk *chunk, Value value, int line);
int add_constant(Chunk *chunk, Value value);
int get_line(Chunk *chunk, int offset);

#endif
