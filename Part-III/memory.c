#include <stdlib.h>

#include "memory.h"
#include "vm.h"

/* reallocate: clox dynamic memory management function. */
void *reallocate(void *pointer, size_t old_size, size_t new_size)
{
    if (new_size == 0) {
        free(pointer);
        return NULL;
    }

    void *result = realloc(pointer, new_size);
    if (result == NULL) exit(1);
    return result;
}

/* free_object: free an objects memory based on its object type. */
static void free_object(Obj *object) {
    switch (object->type) {
        case OBJ_STRING: {
            FREE(ObjString, object);
            break;
        }
    }
}

/* free_objects: walk the object linked list and free its nodes. */
void free_objects() {
    Obj *object = vm.objects;
    while (object != NULL) {
        Obj *next = object->next;
        free_object(object);
        object = next;
    }
}
