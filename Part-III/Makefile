# Compiler
CC = clang

# Executable name
TARGET = clox

# Source files
SRCS = $(wildcard *.c)

# Object files
OBJS = $(SRCS:.c=.o)

# Default rule
all: $(TARGET)

# Rule to link the object files to create the executable
$(TARGET): $(OBJS)
	$(CC) -o $@ $^

# Rule to compile source files into object files
%.o: %.c
	$(CC) -c $< -o $@

# Clean up generated files
clean:
	rm -f $(OBJS) $(TARGET)

# Phony targets
.PHONY: all clean
