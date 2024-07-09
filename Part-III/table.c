#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"

#define TABLE_MAX_LOAD 0.75

/* init_table: initialize a hash table. */
void init_table(Table *table)
{
    table->count = 0;
    table->capacity = 0;
    table->entries = NULL;
}

/* free_table: free a hash table's memory. */
void free_table(Table *table)
{
    FREE_ARRAY(Entry, table->entries, table->capacity);
    init_table(table);
}

/* find_entry: find the appropriate bucket of 'table' into which 'value' belongs. */
static Entry *find_entry(Entry *entries, int capacity, ObjString *key)
{
    uint32_t index = key->hash % capacity;
    Entry *tombstone = NULL;

    for (;;) {
        Entry *entry = &entries[index];
        if (entry->key == NULL) {
            if (IS_NIL(entry->value))
                return tombstone != NULL ? tombstone : entry;
            else
                if (tombstone == NULL) tombstone = entry;
        } else if (entry->key == key)
            return entry;

        index = (index + 1) % capacity;
    }
}

/* table_get: returns true if entry with a specified key is found in the table. */
bool table_get(Table *table, ObjString *key, Value *value)
{
    if (table->count == 0) return false;

    Entry *entry = find_entry(table->entries, table->capacity, key);
    if (entry->key == NULL) return false;

    *value = entry->value;
    return true;
}

/* adjust_capacity: create a bucket array with capacity entries. */
static void adjust_capacity(Table *table, int capacity)
{
    Entry *entries = ALLOCATE(Entry, capacity);
    for (int i = 0; i < capacity; i++) {
        entries[i].key = NULL;
        entries[i].value = NIL_VAL;
    }

    // Re-insert each entry into the new empty array to avoid collisions.
    table->count = 0;
    for (int i = 0; i < table->capacity; i++) {
        Entry *entry = &table->entries[i];
        if (entry->key == NULL) continue;
        Entry *dest = find_entry(entries, capacity, entry->key);
        dest->key = entry->key;
        dest->value = entry->value;
        table->count++;
    }

    FREE_ARRAY(Entry, table->entries, table->capacity);
    table->entries = entries;
    table->capacity = capacity;
}

/* table_set: add a key-value pair to a hash table. */
bool table_set(Table *table, ObjString *key, Value value)
{
    // Grow the table's capacity if necessary.
    if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
        int capacity = GROW_CAPACITY(table->capacity);
        adjust_capacity(table, capacity);
    }

    // Put the new entry in the table.
    Entry *entry = find_entry(table->entries, table->capacity, key);
    bool is_new_key = entry->key == NULL;
    if (is_new_key && IS_NIL(entry->value)) table->count++;

    entry->key = key;
    entry->value = value;
    return is_new_key;    // Return true if the entry was added.
}

/* table_delete: delete an entry from a hash table. */
bool table_delete(Table *table, ObjString *key)
{
    if (table->count == 0) return false;

    // Find the entry.
    Entry *entry = find_entry(table->entries, table->capacity, key);
    if (entry->key == NULL) return false;

    // Place a tombstone (null key, true value) in the entry.
    entry->key = NULL;
    entry->value = BOOL_VAL(true);
    return true;
}

/* table_add_all: copy all entries of one hash table into another. */
void table_add_all(Table *from, Table *to)
{
    for (int i = 0; i < from->capacity; i++) {
        Entry *entry = &from->entries[i];
        if (entry->key != NULL)
            table_set(to, entry->key, entry->value);
    }
}

/* table_find_string: use string interning to find a string in a hash table. */
ObjString *table_find_string(Table *table, const char *chars,
                             int length, uint32_t hash)
{
    if (table->count == 0) return NULL;

    uint32_t index = hash % table->capacity;
    for (;;) {
        Entry *entry = &table->entries[index];
        if (entry->key == NULL) {
            if (IS_NIL(entry->value)) return NULL;
            else if (entry->key->length == length &&
                        entry->key->hash == hash &&
                        memcmp(entry->key->chars, chars, length) == 0)
            return entry->key;
        }

        index = (index + 1) % table->capacity;
    }
}
