#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "chunk.h"
#include "common.h"
#include "compiler.h"
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

/* runtime_error: reports runtime errors to the user. */
static void runtime_error(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    size_t instruction = vm.ip - vm.chunk->code - 1;
    int line = vm.chunk->lines.line_number_entries[instruction].line_number;
    fprintf(stderr, "[line %d] in script\n", line);
    reset_stack();
}

/* init_vm: initialize the virtual machine. */
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

    *vm.stack_top++ = value;
}

/* pop: pop a Value off the stack and return it. */
Value pop()
{
    return *(--vm.stack_top);
}

/* peek: returns a Value from the stack but does not pop it. */
static Value peek(int distance)
{
    return vm.stack_top[-1 - distance];
}

/* is_falsey: nil and false are falsey, everything else behaves like true. */
static bool is_falsey(Value value)
{
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

/* interpret: interpret a chunk of bytecode. */
InterpretResult interpret(const char *source)
{
    Chunk chunk;
    init_chunk(&chunk);

    if (!compile(source, &chunk)) {
        free_chunk(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }

    vm.chunk = &chunk;
    vm.ip = vm.chunk->code;

    InterpretResult result = run();

    free_chunk(&chunk);
    return result;
}

/* run: the VM's beating heart. */
static InterpretResult run()
{

/* READ_BYTE MACRO: Reads one byte as an index into the constant pool. */
#define READ_BYTE() (*vm.ip++)

/* READ_LONG MACRO: Reads three bytes as a 24-bit index into the constant pool. */
#define READ_LONG() (READ_BYTE() | (READ_BYTE() << 8) | (READ_BYTE() << 16))
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define READ_CONSTANT_LONG() (vm.chunk->constants.values[READ_LONG()])

/* ARITHMETIC_OP MACRO: avoids calling push() and pop(). */
#define BINARY_OP(value_type, op)                         \
    do {                                                  \
        if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
            runtime_error("Operands must be numbers.");   \
            return INTERPRET_RUNTIME_ERROR;               \
        }                                                 \
        double b = AS_NUMBER(*(vm.stack_top - 1));        \
        double a = AS_NUMBER(*(vm.stack_top - 2));        \
        *(vm.stack_top - 2) = value_type(a op b);         \
        vm.stack_top--;                                   \
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
            case OP_NIL: push(NIL_VAL); break;
            case OP_TRUE: push(BOOL_VAL(true)); break;
            case OP_FALSE: push(BOOL_VAL(false)); break;
            case OP_EQUAL: {
                *(vm.stack_top - 2) = BOOL_VAL(
                    values_equal(*(vm.stack_top - 1), *(vm.stack_top - 2))
                );
                vm.stack_top--;
                break;
            }
            case OP_GREATER:  BINARY_OP(BOOL_VAL, >); break;
            case OP_LESS:     BINARY_OP(BOOL_VAL, <); break;
            case OP_ADD:      BINARY_OP(NUMBER_VAL, +); break;
            case OP_SUBTRACT: BINARY_OP(NUMBER_VAL, -); break;
            case OP_MULTIPLY: BINARY_OP(NUMBER_VAL, *); break;
            case OP_DIVIDE:   BINARY_OP(NUMBER_VAL, /); break;
            case OP_NOT: {
                *(vm.stack_top - 1) = BOOL_VAL(is_falsey(*(--vm.stack_top)));
                break;
            }
            case OP_NEGATE: {
                if (!IS_NUMBER(peek(0))) {
                    runtime_error("Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                *(vm.stack_top - 1) = NUMBER_VAL(AS_NUMBER(*(vm.stack_top - 1)) * -1);
                break;
            }
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
#undef ARITHMETIC_OP
}
