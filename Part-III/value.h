#ifndef clox_value_h
#define clox_value_h

#include "common.h"

/* Constant value type. */
typedef double Value;

/* Dynamic array structure to hold a chunk's constant pool. */
typedef struct {
    int capacity;
    int count;
    Value *values;
} ValueArray;

void init_value_array(ValueArray *array);
void write_value_array(ValueArray *array, Value value);
void free_value_array(ValueArray *array);
void print_value(Value value);

#endif
