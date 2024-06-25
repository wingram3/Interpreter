#include <stdio.h>

#include "common.h"
#include "debug.h"
#include "vm.h"

VM vm;  // Single global virtual machine object.

/* init_vm: initialize the virtual machine.  */
void init_vm() {}

/* free_vm: free the virtual machine's memory. */
void free_vm() {}

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
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define READ_CONSTANT_LONG() \
    (vm.chunk->constants.values[(READ_BYTE() | (READ_BYTE() << 8) | (READ_BYTE() << 16))])

    // Instruction decoding.
    for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
        disassemble_instruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif

        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
            case OP_CONSTANT: {
                Value constant = READ_CONSTANT();
                print_value(constant);
                printf("\n");
                break;
            }
            case OP_CONSTANT_LONG: {
                Value constant = READ_CONSTANT_LONG();
                print_value(constant);
                printf("\n");
                break;
            }
            case OP_RETURN: {
                return INTERPRET_OK;
            }
        }
    }
#undef READ_BYTE
#undef READ_CONSTANT
#undef READ_CONSTANT_LONG
}
