#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "memory.h"
#include "debug.h"
#include "value.h"
#include "vm.h"

VM vm;  // Single global virtual machine object.

/* reset_stack: sets the stck pointer to the first element in the stack. */
static void reset_stack()
{
    vm.stack_top = vm.stack;
}

/* init_vm: initialize the virtual machine.  */
void init_vm()
{
    vm.stack = (Value *)malloc(INITIAL_STACK_MAX * sizeof(Value));
    reset_stack();
    vm.stack_capacity = INITIAL_STACK_MAX;
}

/* free_vm: free the virtual machine's memory. */
void free_vm()
{
    free(vm.stack);
}

/* push: push a Value onto the stack. */
void push(Value value)
{
    if (vm.stack_top - vm.stack >= vm.stack_capacity) {     // grow the stack size if it is full.
        int old_capacity = vm.stack_capacity;
        vm.stack_capacity = GROW_CAPACITY(old_capacity);
        vm.stack = GROW_ARRAY(Value, vm.stack, old_capacity, vm.stack_capacity);
        vm.stack_top = vm.stack + old_capacity;
    }

    *(vm.stack_top++) = value;
}

/* pop: pop a Value off the stack and return it. */
Value pop()
{
    return *(--vm.stack_top);
}

/* interpret: interpret a chunk of bytecode. */
InterpretResult interpret(Chunk *chunk)
{
    vm.chunk = chunk;
    vm.ip = vm.chunk->code;
    return run();
}

/* run: the VM's beating heart. */
static InterpretResult run()
{
#define READ_BYTE() (*vm.ip++)
#define READ_LONG() (READ_BYTE() | (READ_BYTE() << 8) | (READ_BYTE() << 16))
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define READ_CONSTANT_LONG() (vm.chunk->constants.values[READ_LONG()])

/* BINARY_OP MACRO: avoids pushing and popping from stack to make operations more efficient. */
#define BINARY_OP(op)                            \
    do {                                         \
        vm.stack_top--;                          \
        *(vm.stack_top - 1) op##= *vm.stack_top; \
    } while (false)

    // Instruction decoding.
    for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
        printf("            ");
        for (Value *slot = vm.stack; slot < vm.stack_top; slot++) {
            printf("[ ");
            print_value(*slot);
            printf(" ]");
        }
        printf("\n");
        disassemble_instruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif

        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
            case OP_CONSTANT: {
                Value constant = READ_CONSTANT();
                push(constant);
                break;
            }
            case OP_CONSTANT_LONG: {
                Value constant = READ_CONSTANT_LONG();
                push(constant);
                break;
            }
            case OP_ADD:      BINARY_OP(+); break;
            case OP_SUBTRACT: BINARY_OP(-); break;
            case OP_MULTIPLY: BINARY_OP(*); break;
            case OP_DIVIDE:   BINARY_OP(/); break;
            case OP_NEGATE:
                *(vm.stack_top - 1) *= -1;  // Negate the top stack value in place - faster than 'push(-pop())'
                break;
            case OP_RETURN: {
                print_value(pop());
                printf("\n");
                return INTERPRET_OK;
                break;
            }
        }
    }
#undef READ_BYTE
#undef READ_LONG
#undef READ_CONSTANT
#undef READ_CONSTANT_LONG
#undef BINARY_OP
}