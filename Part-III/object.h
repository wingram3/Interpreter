#ifndef clox_object_h
#define clox_object_h

#include "common.h"
#include "chunk.h"
#include "value.h"

// Macros to extract object types from a given Value.
#define OBJ_TYPE(value)     (AS_OBJ(value)->type)

// Macros to check the object types of objects.
#define IS_FUNCTION(value)  is_obj_type(value, OBJ_FUNCTION)
#define IS_STRING(value)    is_obj_type(value, OBJ_STRING)

// Macros to cast a value to an object type.
#define AS_FUNCTION(value)  ((ObjFunction *)AS_OBJ(value))
#define AS_STRING(value)    ((ObjString *)AS_OBJ(value))
#define AS_CSTRING(value)   (((ObjString *)AS_OBJ(value))->chars)

/* Enum to hold all the object types. */
typedef enum {
    OBJ_STRING,
    OBJ_FUNCTION,
} ObjType;

/* Contains state shared across all object types.
   Object composition - meant to mimic inheritance in OOP. */
struct Obj {
    ObjType type;
    struct Obj *next;   // Points to next obj in the chain for memory management.
};

/* Function object - each function has its own chunk. */
typedef struct {
    Obj obj;
    int arity;          // Number of parameters the function expects.
    Chunk chunk;        // The function's bytecode chunk.
    ObjString *name;    // The name of the function.
} ObjFunction;

/* Payload for string objects. */
struct ObjString {
    Obj obj;
    int length;     // Number of bytes in the character array.
    uint32_t hash;  // Hash code for the string.
    char chars[];   // Flexible array member for the character array.
};

ObjFunction *new_function();
ObjString *take_string(char* chars, int length);
ObjString *copy_string(const char *chars, int length);
void print_object(Value value);

/* is_obj_type: tells when it is safe to cast a value to a specific object type. */
static inline bool is_obj_type(Value value, ObjType type)
{
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif
