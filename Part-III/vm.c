#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chunk.h"
#include "common.h"
#include "compiler.h"
#include "memory.h"
#include "object.h"
#include "memory.h"
#include "value.h"
#include "vm.h"

VM vm;  // Single global virtual machine object.

/* reset_stack: sets the stck pointer to the first element in the stack. */
static void reset_stack()
{
    vm.stack_top = vm.stack;
    vm.frame_count = 0;
}

/* runtime_error: reports runtime errors to the user. */
static void runtime_error(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    for (int i = vm.frame_count - 1; i >= 0; i--) {
        CallFrame *frame = &vm.frames[i];
        ObjFunction *function = frame->function;
        size_t instruction = frame->ip - function->chunk.code - 1;
        fprintf(stderr, "[line %d] in ",
            function->chunk.lines.line_number_entries[instruction].line_number);
        if (function->name == NULL)
            fprintf(stderr, "script\n");
        else
            fprintf(stderr, "%s()\n", function->name->chars);
    }

    reset_stack();
}

/* init_vm: initialize the virtual machine. */
void init_vm()
{
    vm.stack = (Value *)malloc(INITIAL_STACK_MAX * sizeof(Value));
    reset_stack();
    vm.objects = NULL;
    vm.stack_capacity = INITIAL_STACK_MAX;
    init_table(&vm.globals);
    init_table(&vm.strings);
}

/* free_vm: free the virtual machine's memory. */
void free_vm()
{
    free(vm.stack);
    free_objects();
    free_table(&vm.globals);
    free_table(&vm.strings);
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

/* call: call a lox function. */
static bool call(ObjFunction *function, int arg_count)
{
    if (arg_count != function->arity) {
        runtime_error("Expected %d argments but got %d.",
            function->arity, arg_count);
        return false;
    }

    if (vm.frame_count == FRAMES_MAX) {
        runtime_error("Stack overflow.");
        return false;
    }

    CallFrame *frame = &vm.frames[vm.frame_count++];
    frame->function = function;
    frame->ip = function->chunk.code;
    frame->slots = vm.stack_top - arg_count - 1;
    return true;
}

/* call_value: returns true if the thing being called is a function or class, error o/w. */
static bool call_value(Value callee, int arg_count)
{
    if (IS_OBJ(callee))
        switch (OBJ_TYPE(callee)) {
            case OBJ_FUNCTION: return call(AS_FUNCTION(callee), arg_count);
            default: break;
        }
    runtime_error("Can only call functions and classes.");
    return false;
}

/* falsey: returns 1 if a value is falsey, 0 otherwise. */
static int falsey(Value value)
{
    return IS_NIL(value) ||
           AS_NUMBER(value) == 0 ||
           (IS_BOOL(value) && !AS_BOOL(value)) ? 1 : 0;
}

/* is_falsey: nil, zero, and false are falsey, everything else behaves like true. */
static bool is_falsey(Value value)
{
    return IS_NIL(value) ||
           AS_NUMBER(value) == 0 ||
           (IS_BOOL(value) && !AS_BOOL(value));
}

/* concatenate: concatencate two string objects. */
static void concatenate()
{
    ObjString *b = AS_STRING(*(vm.stack_top - 1));
    ObjString *a = AS_STRING(*(vm.stack_top - 2));

    int length = a->length + b->length;
    char *chars = ALLOCATE(char, length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';

    ObjString *result = take_string(chars, length);
    *(vm.stack_top - 2) = OBJ_VAL(result);
    vm.stack_top--;
}

/* interpret: interpret a chunk of bytecode. */
InterpretResult interpret(const char *source)
{
    ObjFunction *function = compile(source);
    if (function == NULL) return INTERPRET_COMPILE_ERROR;

    push(OBJ_VAL(function));
    call(function, 0);

    CallFrame *frame = &vm.frames[vm.frame_count++];
    frame->function = function;
    frame->ip = function->chunk.code;
    frame->slots = vm.stack;

    return run();
}

/* run: the VM's beating heart. */
static InterpretResult run()
{
    CallFrame *frame = &vm.frames[vm.frame_count - 1];

#define READ_BYTE() (*frame->ip++)
#define READ_LONG() (READ_BYTE() | (READ_BYTE() << 8) | (READ_BYTE() << 16))
#define READ_SHORT() (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
#define READ_CONSTANT() (frame->function->chunk.constants.values[READ_BYTE()])
#define READ_CONSTANT_LONG() (frame->function->chunk.constants.values[READ_LONG()])
#define READ_STRING() AS_STRING(READ_CONSTANT())
#define READ_STRING_LONG() AS_STRING(READ_CONSTANT_LONG())
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
        // Print stack trace for debugging.
        printf("            ");
        for (Value *slot = vm.stack; slot < vm.stack_top; slot++) {
            printf("[ ");
            print_value(*slot);
            printf(" ]");
        }
        printf("\n");
        disassemble_instruction(&frame->function->chunk,
            (int)(frame->ip - frame->function->chunk.code));
#endif

        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
            // Read a constant from constant pool, put it on stack.
            case OP_CONSTANT: {
                Value constant = READ_CONSTANT();
                push(constant);
                break;
            }
            // Read a 24-bit constant from constant pool, put it on stack.
            case OP_CONSTANT_LONG: {
                Value constant = READ_CONSTANT_LONG();
                push(constant);
                break;
            }
            // Special push instructions for 1, 2, and 3.
            case OP_ZERO:  push(NUMBER_VAL(0.0)); break;
            case OP_ONE:   push(NUMBER_VAL(1.0)); break;
            case OP_TWO:   push(NUMBER_VAL(2.0)); break;

            // Push nil (null) on to the stack.
            case OP_NIL:   push(NIL_VAL); break;

            // Push true and false on to the stack.
            case OP_TRUE:  push(BOOL_VAL(true)); break;
            case OP_FALSE: push(BOOL_VAL(false)); break;

            // Pop a value from the stack.
            case OP_POP:   pop(); break;

            // Pop multiple values, for when several local vars go out of scope at once.
            case OP_POPN: {
                int count = READ_BYTE();
                while (count-- > 0) pop();
                break;
            }
            // Get a global from globals hash table, put it on stack.
            case OP_GET_GLOBAL: {
                ObjString *name = READ_STRING();
                Value value;
                if (!table_get(&vm.globals, name, &value)) {
                    runtime_error("Undefined variable '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(value);
                break;
            }
            // Get a 24-bit global from globals hash table, put it on stack.
            case OP_GET_GLOBAL_LONG: {
                ObjString *name = READ_STRING_LONG();
                Value value;
                if (!table_get(&vm.globals, name, &value)) {
                    runtime_error("Undefined variable '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(value);
                break;
            }
            // Store the top stack value into globals hash table according to its key.
            case OP_SET_GLOBAL: {
                ObjString *name = READ_STRING();
                if (table_set(&vm.globals, name, peek(0))) {
                    table_delete(&vm.globals, name);
                    runtime_error("Undefined variable '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            // Store the top stack value (24-bit) into globals hash table according to its key.
            case OP_SET_GLOBAL_LONG: {
                ObjString *name = READ_STRING_LONG();
                if (table_set(&vm.globals, name, peek(0))) {
                    table_delete(&vm.globals, name);
                    runtime_error("Undefined variable '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            // Push a local variable's value on to the stack.
            case OP_GET_LOCAL: {
                uint8_t slot = READ_BYTE();
                push(frame->slots[slot]);
                break;
            }
            // Store a local, the value on top of stack becomes the local's value.
            case OP_SET_LOCAL: {
                uint8_t slot = READ_BYTE();
                frame->slots[slot] = peek(0);
                break;
            }
            // Define a global variable. Put its key and value in globals hash table.
            case OP_DEFINE_GLOBAL: {
                ObjString *name = READ_STRING();
                table_set(&vm.globals, name, peek(0));
                pop();
                break;
            }
            // Define a 24-bit global variable. Put its key and value in globals hash table.
            case OP_DEFINE_GLOBAL_LONG: {
                ObjString *name = READ_STRING_LONG();
                table_set(&vm.globals, name, peek(0));
                pop();
                break;
            }
            // Check if top two stack values are equal, push true or false accordingly.
            case OP_EQUAL: {
                *(vm.stack_top - 2) = BOOL_VAL(
                    values_equal(*(vm.stack_top - 2), *(vm.stack_top - 1))
                );
                vm.stack_top--;
                break;
            }
            // Opposite of OP_EQUAL.
            case OP_NOT_EQUAL: {
                *(vm.stack_top - 2) = BOOL_VAL(
                    !values_equal(*(vm.stack_top - 2), *(vm.stack_top - 1))
                );
                vm.stack_top--;
                break;
            }
            // Binary operations for comparison between numbers.
            case OP_GREATER:        BINARY_OP(BOOL_VAL, >); break;
            case OP_GREATER_EQUAL:  BINARY_OP(BOOL_VAL, >=); break;
            case OP_LESS:           BINARY_OP(BOOL_VAL, <); break;
            case OP_LESS_EQUAL:     BINARY_OP(BOOL_VAL, <=); break;

            // Add two numbers or concatenate two strings.
            case OP_ADD: {
                if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
                    concatenate();
                } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
                    double b = AS_NUMBER(*(vm.stack_top - 1));
                    double a = AS_NUMBER(*(vm.stack_top - 2));
                    *(vm.stack_top - 2) = NUMBER_VAL(a + b);
                    vm.stack_top--;
                } else {
                    runtime_error("Operands must be two numbers or two strings.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            // Binary arithmetic operations for two numbers.
            case OP_SUBTRACT:       BINARY_OP(NUMBER_VAL, -); break;
            case OP_MULTIPLY:       BINARY_OP(NUMBER_VAL, *); break;
            case OP_DIVIDE:         BINARY_OP(NUMBER_VAL, /); break;

            // Unary invert operation. Effectively pushes opposite of stack top's bool value.
            case OP_NOT: {
                *(vm.stack_top - 1) = BOOL_VAL(is_falsey(*(vm.stack_top - 1)));
                break;
            }
            // Unary negate operation.
            case OP_NEGATE: {
                if (!IS_NUMBER(peek(0))) {
                    runtime_error("Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                *(vm.stack_top - 1) = NUMBER_VAL(AS_NUMBER(*(vm.stack_top - 1)) * -1);
                break;
            }
            // Jump back to top of loop.
            case OP_LOOP: {
                uint16_t offset = READ_SHORT();
                frame->ip -= offset;
                break;
            }
            // Unconditional jump instruction.
            case OP_JUMP: {
                uint16_t offset = READ_SHORT();
                frame->ip += offset;
                break;
            }
            // Jump if stack top evaluates to true.
            case OP_JUMP_IF_TRUE: {
                uint16_t offset = READ_SHORT();
                frame->ip += !falsey(*(vm.stack_top - 1)) * offset;
                break;
            }
            // Jump if stack top evaluates to false.
            case OP_JUMP_IF_FALSE: {
                uint16_t offset = READ_SHORT();
                frame->ip += falsey(*(vm.stack_top - 1)) * offset;
                break;
            }
            // Jump if top two stack values are not equal.
            case OP_JUMP_NOT_EQUAL: {
                uint16_t offset = READ_SHORT();
                Value first_value = pop();
                Value second_value = peek(0);
                if (!values_equal(second_value, first_value))
                    frame->ip += offset;
                else pop();
                break;
            }
            // Print the value on top of stack.
            case OP_PRINT: {
                print_value(pop());
                printf("\n");
                break;
            }
            //
            case OP_CALL: {
                int arg_count = READ_BYTE();
                if (!call_value(peek(arg_count), arg_count))
                    return INTERPRET_RUNTIME_ERROR;
                frame = &vm.frames[vm.frame_count - 1];
                break;
            }
            // Return instruction.
            case OP_RETURN: {
                Value result = pop();
                vm.frame_count--;
                if (vm.frame_count == 0) {
                    pop();
                    return INTERPRET_OK;
                }

                vm.stack_top = frame->slots;
                push(result);
                frame = &vm.frames[vm.frame_count - 1];
                break;
            }
        }
    }
#undef READ_BYTE
#undef READ_LONG
#undef READ_SHORT
#undef READ_CONSTANT
#undef READ_CONSTANT_LONG
#undef READ_STRING
#undef READ_STRING_LONG
#undef BINARY_OP
}
