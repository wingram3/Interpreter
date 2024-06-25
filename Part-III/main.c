#include "common.h"
#include "chunk.h"
#include "vm.h"

int main(int argc, char *argv[])
{
    init_vm();

    Chunk chunk;
    init_chunk(&chunk);

    for (int i = 0; i < 500; i += 20) {
        write_constant(&chunk, i, 123);
    }
    write_chunk(&chunk, OP_RETURN, 123);

    interpret(&chunk);

    free_vm();
    free_chunk(&chunk);

    return 0;
}
