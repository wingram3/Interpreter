#ifndef clox_table_h
#define clox_table_h

#include "common.h"
#include "value.h"

/* Key-value pair structure. */
typedef struct {
    ObjString *key;
    Value value;
} Entry;

/* Hash table structure. */
typedef struct {
    int count;
    int capacity;
    Entry *entries;
} Table;

void init_table(Table *table);
void free_table(Table *table);
bool table_set(Table *table, ObjString *key, Value value);

#endif
