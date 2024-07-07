#ifndef clox_value_h
#define clox_value_h

#include "common.h"

typedef struct Obj Obj;
typedef struct ObjString ObjString;

/* Enum to hold types of values. */
typedef enum {
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER,
    VAL_OBJ,
} ValueType;

/* Tagged union to hold a Value's type tag and its actual value. */
typedef struct {
    ValueType type;
    union {
        bool boolean;
        double number;
        Obj *obj;
    } as;
} Value;

/* Macros to check a clox Value's type before the AS_ macros are called. */
#define IS_BOOL(value)   ((value).type == VAL_BOOL)
#define IS_NIL(value)    ((value).type == VAL_NIL)
#define IS_NUMBER(value) ((value).type == VAL_NUMBER)
#define IS_OBJ(value)    ((value).type == VAL_OBJ)

/* Macros to unpack a clox Value and get the C value back out. */
#define AS_OBJ(value)    ((value).as.obj)
#define AS_BOOL(value)   ((value).as.boolean)
#define AS_NUMBER(value) ((value).as.number)

/* Macros to promote a native C value to a clox Value. */
#define BOOL_VAL(value)   ((Value){VAL_BOOL, {.boolean = value}})
#define NIL_VAL           ((Value){VAL_NIL, {.number = 0}})
#define NUMBER_VAL(value) ((Value){VAL_NUMBER, {.number = value}})
#define OBJ_VAL(object)   ((Value){VAL_OBJ, {.obj = (Obj *)object}})

/* Dynamic array structure to hold a chunk's constant pool. */
typedef struct {
    int capacity;
    int count;
    Value *values;
} ValueArray;

bool values_equal(Value a, Value b);
bool values_greater_equal(Value a, Value b);
bool values_less_equal(Value a, Value b);
bool values_not_equal(Value a, Value b);
void init_value_array(ValueArray *array);
void write_value_array(ValueArray *array, Value value);
void free_value_array(ValueArray *array);
void print_value(Value value);

#endif
