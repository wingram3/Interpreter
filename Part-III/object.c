#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
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

/* allocate_string: creates a new ObjString on the heap and then initializes its fields. */
static ObjString *allocate_string(const char *chars, int length, uint32_t hash)
{
    ObjString *string = (ObjString *)allocate_object(
        sizeof(ObjString) + length + 1, OBJ_STRING);

    string->length = length;
    memcpy(string->chars, chars, length);
    string->chars[length] = '\0';

    string->hash = hash;

    return string;
}

/* hash_string: FNV-1a hash function. */
static uint32_t hash_string(const char *key, int length)
{
    uint32_t hash = 216613626u;
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
    return allocate_string(chars, length, hash);
}

/* take_string: claims ownership of the string that is given to it. */
ObjString *copy_string(const char *chars, int length)
{
    uint32_t hash = hash_string(chars, length);
    return allocate_string(chars, length, hash);
}

/* print_object: displays an object's value. */
void print_object(Value value)
{
    switch (OBJ_TYPE(value)) {
        case OBJ_STRING:
        printf("%s", AS_CSTRING(value));
        break;
    }
}
