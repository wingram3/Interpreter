# Define the Java compiler and flags
JAVAC = javac
JFLAGS = -g

# Define the source files
SOURCES = com/lox/*.java

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

# Generate AST classes (Stmt.java & Expr.java)
ast:
	javac com/tool/GenerateAst.java
	java com.tool.GenerateAst com/lox

# Clean up generated class files
clean:
	rm -f com/lox/*.class com/tool/*.class
