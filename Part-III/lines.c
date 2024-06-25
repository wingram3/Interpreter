#include <stdlib.h>

#include "lines.h"
#include "memory.h"

/* init_line_number_array: initialize an array of line numbers. */
void init_line_number_array(LineNumberArray *array)
{
    array->capacity = 0;
    array->count = 0;
    array->line_number_entries = NULL;
}

/* write_line_number_array: write a line number mapping to line number array. */
void write_line_number_array(LineNumberArray *array, LineNumberEntry line)
{
    if (array->capacity < array->count + 1) {
        int old_capacity = array->capacity;
        array->capacity = GROW_CAPACITY(old_capacity);
        array->line_number_entries = GROW_ARRAY(LineNumberEntry, array->line_number_entries,
            old_capacity, array->capacity);
    }

    array->line_number_entries[array->count] = line;
    array->count++;
}

/* free_line_number_array: free the line number array. */
void free_line_number_array(LineNumberArray *array)
{
    FREE_ARRAY(LineNumberEntry, array->line_number_entries, array->capacity);
    init_line_number_array(array);
}
