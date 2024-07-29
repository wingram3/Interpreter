#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"
#include "vm.h"

/* ALLOCATE_OBJ MACRO: allocates an object of the given size on the heap. */
#define ALLOCATE_OBJ(type, objectType) \
    (type *)allocate_object(sizeof(type), objectType)

/* allocate_object: allocates an object of the given size on the heap. */
static Obj *allocate_object(size_t size, ObjType type) {
    Obj *object = (Obj *)reallocate(NULL, 0, size);
    object->type = type;

    object->next = vm.objects;
    vm.objects = object;
    return object;
}

/* new_function: creates a new ObjFunction on the heap and initializes its fields. */
ObjFunction *new_function()
{
    ObjFunction *function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
    function->arity = 0;
    function->name = NULL;
    init_chunk(&function->chunk);
    return function;
}

/* allocate_string: creates a new ObjString on the heap and initializes its fields. */
static ObjString *allocate_string(int length, uint32_t hash)
{
    ObjString *string = (ObjString *)allocate_object(
        sizeof(ObjString) + length + 1, OBJ_STRING);
    string->length = length;
    string->hash = hash;
    return string;
}

/* hash_string: FNV-1a hash function. */
static uint32_t hash_string(const char* key, int length)
{
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; i++) {
        hash ^= (uint8_t)key[i];
        hash *= 16777619;
    }
    return hash;
}

/* take_string: claims ownership of the string that is given to it. */
ObjString *take_string(char *chars, int length)
{
    uint32_t hash = hash_string(chars, length);

    ObjString *interned = table_find_string(&vm.strings, chars, length, hash);
    if (interned != NULL) {
        FREE_ARRAY(char, chars, length + 1);
        return interned;
    }

    ObjString *string = allocate_string(length, hash);
    memcpy(string->chars, chars, length);
    string->chars[length] = '\0';

    table_set(&vm.strings, string, NIL_VAL);
    return string;
}

/* copy_string: allocate a new string from the source code on the heap. */
ObjString *copy_string(const char *chars, int length)
{
    uint32_t hash = hash_string(chars, length);

    ObjString *interned = table_find_string(&vm.strings, chars, length, hash);
    if (interned != NULL) return interned;

    ObjString *string = allocate_string(length, hash);
    memcpy(string->chars, chars, length);
    string->chars[length] = '\0';

    table_set(&vm.strings, string, NIL_VAL);
    return string;
}

/* print_function: displays a function's name. */
static void print_function(ObjFunction *function)
{
    if (function->name == NULL) {
        printf("<script>");
        return;
    }
    printf("<fn %s>", function->name->chars);
}

/* print_object: displays an object's value. */
void print_object(Value value)
{
    switch (OBJ_TYPE(value)) {
        case OBJ_STRING: {
            printf("%s", AS_CSTRING(value));
            break;
        }
        case OBJ_FUNCTION: {
            print_function(AS_FUNCTION(value));
            break;
        }
    }
}
