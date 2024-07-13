#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "lines.h"
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
    OP_PRINT,
    OP_RETURN,
} OpCode;

/* Dynamic array structure to hold sequences of bytecode. */
typedef struct {
    int count;              // number of allocated entries in use.
    int capacity;           // total number of allocated entires.
    uint8_t *code;          // an array of bytes.
    LineNumberArray lines;  // line numbers - mappings of bytecode offsets to source code line numbers.
    ValueArray constants;   // constants associated w/ the chunk.
} Chunk;

void init_chunk(Chunk *chunk);
void free_chunk(Chunk *chunk);
void write_chunk(Chunk *chunk, uint8_t byte, int line);
void write_constant(Chunk *chunk, Value value, int line);
int add_constant(Chunk *chunk, Value value);
void add_line(Chunk *chunk, int line);

#endif
