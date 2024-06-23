#include <stdio.h>

#include "debug.h"
#include "value.h"

/* disassemble_chunk: disassemble all instructions in a chunk. */
void disassemble_chunk(Chunk *chunk, const char *name)
{
    printf("== %s ==\n", name);
    for (int offset = 0; offset < chunk->count;) {
        offset = disassemble_instruction(chunk, offset);
    }
}

/* disassemble_instruction: disassemble a single instruction. */
int disassemble_instruction(Chunk *chunk, int offset)
{
    printf("%04d ", offset);
    if (offset > 0 && chunk->lines[offset] == chunk->lines[offset - 1])
        printf("   | ");
    else
        printf("%4d ", chunk->lines[offset]);

    uint8_t instruction = chunk->code[offset];
    switch (instruction) {
        case OP_RETURN:
            return simple_instruction("OP_RETURN", offset);
        case OP_CONSTANT:
            return constant_instruction("OP_CONSTANT", chunk, offset);
        default:
            printf("Unknown opcode %d\n", instruction);
            return offset + 1;
    }
}

/* simple_instruction: display the opcode at an offset. */
static int simple_instruction(const char *name, int offset)
{
    printf("%s\n", name);
    return offset + 1;
}

/* constant_instruction: display the opcode at an offset w/ its constant value. */
static int constant_instruction(const char *name, Chunk *chunk, int offset)
{
    uint8_t constant = chunk->code[offset + 1];
    printf("%-16s %4d '", name, constant);
    print_value(chunk->constants.values[constant]);
    printf("'\n");
    return offset + 2;
}
