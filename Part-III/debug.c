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
    if (offset > 0 && get_line(&chunk->lines, offset)
            == get_line(&chunk->lines, offset - 1))
        printf("   | ");
    else
        printf("%4d ", get_line(&chunk->lines, offset));

    uint8_t instruction = chunk->code[offset];
    switch (instruction) {
        case OP_RETURN:
            return simple_instruction("OP_RETURN", offset);
        case OP_CONSTANT:
            return constant_instruction("OP_CONSTANT", chunk, offset);
        case OP_CONSTANT_LONG:
            return constant_long_instruction("OP_CONSTANT_LONG", chunk, offset);
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

/* constant_long_instruction: display the opcode at an offset w/ its constant value. */
static int constant_long_instruction(const char *name, Chunk *chunk, int offset)
{
    uint32_t constant = chunk->code[offset + 1] |
                        (chunk->code[offset + 2] << 8) |
                        (chunk->code[offset + 3] << 16);
    printf("%-16s %4d '", name, constant);
    print_value(chunk->constants.values[constant]);
    printf("'\n");
    return offset + 4;
}

/* get_line: get the source line number from a given bytecode offset. */
int get_line(LineNumberArray *lines, int offset)
{
    for (int i = lines->count - 1; i >= 0; i--) {
        if (lines->line_number_entries[i].bytecode_offset <= offset)
            return lines->line_number_entries[i].line_number;
    }
    return -1;
}
