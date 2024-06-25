#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"

int main(int argc, char *argv[])
{
    init_vm();

    Chunk chunk;
    init_chunk(&chunk);

    write_constant(&chunk, 16000000, 123);
    write_constant(&chunk, 1.2, 123);
    write_constant(&chunk, 100000.4, 123);
    write_constant(&chunk, 0.3, 123);

    write_chunk(&chunk, OP_RETURN, 123);
    interpret(&chunk);
    free_vm();
    free_chunk(&chunk);
    return 0;
}
