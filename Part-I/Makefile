# Define the Java compiler and flags
JAVAC = javac
JFLAGS = -g

# Define the source files
SOURCES = com/lox/Lox.java com/lox/Scanner.java com/lox/Token.java com/lox/TokenType.java com/tool/GenerateAst.java
# Define the class files (object files)
CLASSES = $(SOURCES:.java=.class)

# Default target
all: $(CLASSES)

# Rule to compile Java source files to class files
$(CLASSES): $(SOURCES)
	$(JAVAC) $(JFLAGS) $(SOURCES)

# Run the REPL
run: all
	java -cp . com.lox.Lox

# Clean up generated class files
clean:
	rm -f com/lox/*.class com/tool/*.class
