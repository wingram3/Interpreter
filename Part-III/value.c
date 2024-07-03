#include <stdio.h>

#include "memory.h"
#include "value.h"

/* init_value_array: initialize an array of values. */
void init_value_array(ValueArray *array)
{
    array->capacity = 0;
    array->count = 0;
    array->values = NULL;
}

/* write_value_array: add values to a value array. */
void write_value_array(ValueArray *array, Value value)
{
    if (array->capacity < array->count + 1) {
        int old_capacity = array->capacity;
        array->capacity = GROW_CAPACITY(old_capacity);
        array->values = GROW_ARRAY(Value, array->values,
                                   old_capacity, array->capacity);
    }

    array->values[array->count] = value;
    array->count++;
}

/* free_value_array: release memory used by value array. */
void free_value_array(ValueArray *array)
{
    FREE_ARRAY(Value, array->values, array->capacity);
    init_value_array(array);
}

/* print_value: display a value. */
void print_value(Value value)
{
    printf("%g", AS_NUMBER(value));
}
