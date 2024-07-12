#ifndef clox_debug_h
#define clox_debug_h

#include "chunk.h"

void disassemble_chunk(Chunk *chunk, const char *name);
int disassemble_instruction(Chunk *chunk, int offset);
static int simple_instruction(const char *name, int offset);
static int byte_instruction(const char *name, Chunk *chunk, int offset);
static int constant_instruction(const char *name, Chunk *chunk, int offset);
static int constant_long_instruction(const char *name, Chunk *chunk, int offset);
int get_line(LineNumberArray *lines, int offset);

#endif
