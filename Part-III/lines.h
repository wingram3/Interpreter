#ifndef clox_lines_h
#define clox_lines_h

#include "common.h"

/* Line number entry structure - maps a bytecode offset to source code line number. */
typedef struct {
    int bytecode_offset;
    int line_number;
} LineNumberEntry;

/* Dynamic array structure to hold line number entries. */
typedef struct {
    int capacity;
    int count;
    LineNumberEntry *line_number_entries;
} LineNumberArray;

void init_line_number_array(LineNumberArray *array);
void write_line_number_array(LineNumberArray *array, LineNumberEntry line);
void free_line_number_array(LineNumberArray *array);

#endif
