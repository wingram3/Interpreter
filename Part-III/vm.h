#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"

/* Virtual machine structure. */
typedef struct {
    Chunk *chunk;   // The chunk that the vm executes.
    uint8_t *ip;    // Instruction pointer.
} VM;

/* Enum to hold exit code values. */
typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR,
} InterpretResult;

void init_vm();
void free_vm();
InterpretResult interpret(Chunk *chunk);
static InterpretResult run();

#endif
