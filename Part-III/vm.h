#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"
#include "table.h"
#include "value.h"

#define INITIAL_STACK_MAX 256

/* Virtual machine structure. */
typedef struct {
    Chunk *chunk;           // The chunk that the vm executes.
    uint8_t *ip;            // Instruction pointer.
    Value *stack;           // Dynamic stack array.
    Value *stack_top;       // Points just beyond the last element in the stack.
    Table globals;          // Hash table to store global variables.
    Table strings;          // Hash table to store strings for string interning.
    int stack_capacity;     // Max capacity of the stack - dynamically changes as needed.
    Obj *objects;           // Linked-list of every object.
} VM;

/* Enum to hold exit code values. */
typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR,
} InterpretResult;

extern VM vm;

void init_vm();
void free_vm();
InterpretResult interpret(const char *source);
static InterpretResult run();
void push(Value value);
Value pop();

#endif
