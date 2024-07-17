#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "chunk.h"
#include "common.h"
#include "compiler.h"
#include "scanner.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

#define UINT24_MAX  16777216
#define MAX_CASES   100

/* Global variable to track the current loop's
   increment or start position (for continue stmt). */
int current_continue_jump = -1;

typedef struct {
    Token current;
    Token previous;
    bool had_error;
    bool panic_mode;
} Parser;

/* Precedence levels in order of lowest to highest. */
typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,  // =
    PREC_TERNARY,     // ?:
    PREC_OR,          // or
    PREC_AND,         // and
    PREC_EQUALITY,    // == !=
    PREC_COMPARISON,  // < > <= >=
    PREC_TERM,        // + -
    PREC_FACTOR,      // * /
    PREC_UNARY,       // ! -
    PREC_CALL,        // . ()
    PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)(bool can_assign);

/* Structure to represent a single row in the parser table. */
typedef struct {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;

/* Local variable structure. */
typedef struct {
    Token name;
    int depth;
} Local;

/* Flat array of all local variables that are in scope. */
typedef struct {
    Local locals[UINT8_COUNT];
    int local_count;
    int scope_depth;
} Compiler;

Parser parser;
Compiler *current = NULL;
Chunk *compiling_chunk;

/* current_chunk:  */
static Chunk *current_chunk()
{
    return compiling_chunk;
}

/* error_at: print the line where the error occurred, the lexeme if possible,
   and the error message. Set the had_error flag on the parser. */
static void error_at(Token *token, const char *message)
{
    if (parser.panic_mode) return;
    parser.panic_mode = true;
    fprintf(stderr, "[line %d] Error", token->line);

    if (token->type == TOKEN_EOF)
        fprintf(stderr, " at end");
    else if (token->type == TOKEN_ERROR) {}
    else
        fprintf(stderr, " at '%.*s'", token->length, token->start);

    fprintf(stderr, ": %s\n", message);
    parser.had_error = true;
}

/* error_at_current: error at the current token. */
static void error_at_current(const char *message)
{
    error_at(&parser.current, message);
}

/* error: error at the token that was jsut consumed. */
static void error(const char *message)
{
    error_at(&parser.previous, message);
}

/* advance: step forward through a token stream. */
static void advance()
{
    parser.previous = parser.current;

    for (;;) {
        parser.current = scan_token();
        if (parser.current.type != TOKEN_ERROR) break;

        error_at_current(parser.current.start);
    }
}

/* consume: read the next token in the stream. */
static void consume(TokenType type, const char *message)
{
    if (parser.current.type == type) {
        advance();
        return;
    }

    error_at_current(message);
}

/* check: returns true if the current token matches a given token type. */
static bool check(TokenType type)
{
    return parser.current.type == type;
}

/* match: advances and returns true if a token matches a given token type. */
static bool match(TokenType type)
{
    if (!check(type)) return false;
    advance();
    return true;
}

/* emit_byte: append a single byte to the chunk. */
static void emit_byte(int byte)
{
    write_chunk(current_chunk(), (uint8_t)byte, parser.previous.line);
}

/* emit_bytes: append an arbitary number of bytes to the chunk.
               usage: emit_bytes(byte1, byte2, byte3, ..., -1); */
static void emit_bytes(int first_byte, ...)
{
    va_list args;
    va_start(args, first_byte);
    emit_byte(first_byte);

    int byte;
    while ((byte = va_arg(args, int)) != -1)  // -1 is the sentinel value.
        emit_byte(byte);

    va_end(args);
}

/* emit_loop:  */
static void emit_loop(int loop_start)
{
    emit_byte(OP_LOOP);

    int offset = current_chunk()->count - loop_start + 2;
    if (offset > UINT16_MAX) error("Loop body too large.");

    emit_byte((offset >> 8) & 0xff);
    emit_byte(offset & 0xff);
}

/* emit_jump: emit jump instruction and placeholder operand, return offset. */
static int emit_jump(int instruction)
{
    emit_bytes(instruction, 0xFF, 0xFF, -1);    //
    return current_chunk()->count - 2;
}

/* emit_return: emit a return opcode. */
static void emit_return()
{
    emit_byte(OP_RETURN);
}

/* make_constant: insert an entry into the constant pool. */
static int make_constant(Value value)
{
    int constant = add_constant(current_chunk(), value);
    if (constant > UINT24_MAX) {
        error("Too many constants in one chunk.");
        return 0;
    }

    return constant;
}

/* emit_constant_long: emit a 24-bit constant over three bytes to the chunk. */
static void emit_constant_24bit(int number)
{
    emit_bytes(number & 0xFF, (number >> 8) & 0xFF, (number >> 16) & 0xFF, -1);
}

/* emit_constant: emit a constant value. */
static void emit_constant(Value value)
{
    int constant = make_constant(value);
    if (constant < 256)
        emit_bytes(OP_CONSTANT, constant, -1);
    else {
        emit_byte(OP_CONSTANT_LONG);
        emit_constant_24bit(constant);
    }
}

/* patch_jump: go back into bytecode, replace placeholder jump operand. */
static void patch_jump(int offset)
{
    // -2 to adjust for the bytecode for the jump offset itself.
    int jump = current_chunk()->count - offset - 2;

    if (jump > UINT16_MAX)
        error("Too much code to jump over.");

    current_chunk()->code[offset] = (jump >> 8) & 0xFF;
    current_chunk()->code[offset + 1] = jump & 0xFF;
}

/* init_compiler: initialize the compiler. */
static void init_compiler(Compiler *compiler)
{
    compiler->local_count = 0;
    compiler->scope_depth = 0;
    current = compiler;
}

/* end_compiler: emit a return opcode instruction. */
static void end_compiler()
{
    emit_return();
#ifdef DEBUG_PRINT_CODE
    if (!parser.had_error)
        disassemble_chunk(current_chunk(), "code");
#endif
}

/* begin_scope: enter a new local scope. */
static void begin_scope()
{
    current->scope_depth++;
}

/* end_scope: end a local scope. */
static void end_scope()
{
    int local_count_to_pop = 0;
    current->scope_depth--;

    while (current->local_count > 0 &&
           current->locals[current->local_count - 1].depth >
           current->scope_depth) {
        local_count_to_pop++;
        current->local_count--;
    }

    if (local_count_to_pop > 1) {
        emit_bytes(OP_POPN, local_count_to_pop, -1);
    } else if (local_count_to_pop == 1) {
        emit_byte(OP_POP);
    }
}

static void expression();
static void statement();
static void declaration();
static ParseRule *get_rule(TokenType type);
static void parse_precedence(Precedence precedence);

/* identifier_constant: adds token's lexeme to the chunk’s const. table as string. */
static int identifier_constant(Token *name)
{
    return make_constant(OBJ_VAL(copy_string(name->start,
                                             name->length)));
}

/* identifiers_equal: return true if two identifiers are the same. */
static bool identifiers_equal(Token *a, Token *b)
{
    if (a->length != b->length) return false;
    return memcmp(a->start, b->start, a->length) == 0;
}

/* resolve_local: resolve a local variable. */
static int resolve_local(Compiler *compiler, Token *name)
{
    for (int i = compiler->local_count - 1; i >= 0 ; i--) {
        Local *local = &compiler->locals[i];
        if (identifiers_equal(name, &local->name)) {
            if (local->depth == -1)
                error("Can't read local variable in its own initializer.");
            return i;
        }
    }
    return -1;
}

/* add_local: */
static void add_local(Token name)
{
    if (current->local_count == UINT8_COUNT) {
        error("Too many local variables in function.");
        return;
    }

    Local *local = &current->locals[current->local_count++];
    local->name = name;
    local->depth = -1;
}

/* declare_variable:  */
static void declare_variable()
{
    if (current->scope_depth == 0) return;

    Token *name = &parser.previous;
    for (int i = current->local_count - 1; i >= 0; i--) {
        Local *local = &current->locals[i];
        if (local->depth != -1 && local->depth < current->scope_depth)
            break;

        if (identifiers_equal(name, &local->name))
            error("Already a variable with this name in this scope.");
    }

    add_local(*name);
}

/* parse_variable: uses identifier_constant(). */
static int parse_variable(const char *error_message)
{
    consume(TOKEN_IDENTIFIER, error_message);

    declare_variable();
    if (current->scope_depth > 0) return 0;   // Exit if in a local scope.

    return identifier_constant(&parser.previous);
}

/* mark_initialized: mark a variable as initialized. */
static void mark_initialized()
{
    current->locals[current->local_count - 1].depth =
        current->scope_depth;
}

/* define_variable: outputs the bytecode instruction that defines the new variable,
                    stores its initial value. */
static void define_variable(int global)
{
    if (current->scope_depth > 0) {
        mark_initialized();
        return;
    }

    if (global < 256)
        emit_bytes(OP_DEFINE_GLOBAL, global, -1);
    else {
        emit_byte(OP_DEFINE_GLOBAL_LONG);
        emit_constant_24bit(global);
    }
}

/* logic_and: function for compiling logical and expressions. */
static void logic_and(bool can_assign)
{
    int end_jump = emit_jump(OP_JUMP_IF_FALSE);

    emit_byte(OP_POP);
    parse_precedence(PREC_AND);

    patch_jump(end_jump);
}

/* parse_precedence: starts at current token and parses any expr
                     at the given precedence level or higher. */
static void parse_precedence(Precedence precedence)
{
    advance();
    ParseFn prefix_rule = get_rule(parser.previous.type)->prefix;
    if (prefix_rule == NULL) {
        error("Expect expression.");
        return;
    }

    bool can_assign = precedence <= PREC_ASSIGNMENT;
    prefix_rule(can_assign);

    while (precedence <= get_rule(parser.current.type)->precedence) {
        advance();
        ParseFn infix_rule = get_rule(parser.previous.type)->infix;
        infix_rule(can_assign);
    }

    if (can_assign && match(TOKEN_EQUAL))
        error("Invalid assignment target.");
}

/* binary: function for compiling binary expressions. */
static void binary(bool can_assign)
{
    TokenType operator_type = parser.previous.type;
    ParseRule *rule = get_rule(operator_type);
    parse_precedence((Precedence)(rule->precedence + 1));

    switch (operator_type) {
        case TOKEN_PLUS:          emit_byte(OP_ADD); break;
        case TOKEN_MINUS:         emit_byte(OP_SUBTRACT); break;
        case TOKEN_BANG_EQUAL:    emit_byte(OP_NOT_EQUAL); break;
        case TOKEN_EQUAL_EQUAL:   emit_byte(OP_EQUAL); break;
        case TOKEN_GREATER:       emit_byte(OP_GREATER); break;
        case TOKEN_GREATER_EQUAL: emit_byte(OP_GREATER_EQUAL); break;
        case TOKEN_LESS:          emit_byte(OP_LESS); break;
        case TOKEN_LESS_EQUAL:    emit_byte(OP_LESS_EQUAL); break;
        case TOKEN_STAR:          emit_byte(OP_MULTIPLY); break;
        case TOKEN_SLASH:         emit_byte(OP_DIVIDE); break;
        default: return;
    }
}

/* literal: function for compiling true, false, and nil. */
static void literal(bool can_assign)
{
    switch (parser.previous.type) {
        case TOKEN_FALSE: emit_byte(OP_FALSE); break;
        case TOKEN_NIL: emit_byte(OP_NIL); break;
        case TOKEN_TRUE: emit_byte(OP_TRUE); break;
        default: return;
    }
}

/* ternary: function for compiling ternary (conditional) expressions. */
static void ternary(bool can_assign)
{
    int else_jump = emit_jump(OP_JUMP_IF_FALSE);
    emit_byte(OP_POP);  // Pop the condition expression's value from the stack.
    expression();       // Then expression.

    int end_jump = emit_jump(OP_JUMP);
    patch_jump(else_jump);

    emit_byte(OP_POP);  // Pop the condition expression's value from the stack.

    consume(TOKEN_COLON, "Expect ':' after then branch of ternary expression.");
    expression();       // Else expression.

    patch_jump(end_jump);
}

/* grouping: function for compiling grouping expressions. */
static void grouping(bool can_assign)
{
    expression();   // this inner call handles bytecode generation for the expr in parentheses.
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

/* number: function for compiling number literal expressions. */
static void number(bool can_assign)
{
    double value = strtod(parser.previous.start, NULL);

    switch ((int)value) {
        case 0:
            if (value == 0.0) {
                emit_byte(OP_ZERO);
                break;
            }
        case 1:
            if (value == 1.0) {
                emit_byte(OP_ONE);
                break;
            }
        case 2:
            if (value == 2.0) {
                emit_byte(OP_TWO);
                break;
            }
        default: emit_constant(NUMBER_VAL(value));
    }
}

/* logic_or: function for compiling logical or expressions. */
static void logic_or(bool can_assign)
{
    int end_jump = emit_jump(OP_JUMP_IF_TRUE);
    emit_byte(OP_POP);
    parse_precedence(PREC_OR);
    patch_jump(end_jump);
}

/* string: function for compiling strings. */
static void string(bool can_assign)
{
    emit_constant(OBJ_VAL(copy_string(parser.previous.start + 1,
                                      parser.previous.length - 2)));
}

/* named_variable: take given identifier token, add its lexeme to
                   the chunk’s constant table as a string. */
static void named_variable(Token name, bool can_assign)
{
    int arg = resolve_local(current, &name);
    uint8_t get_op = (arg != -1) ? OP_GET_LOCAL : (arg < 256 ? OP_GET_GLOBAL : OP_GET_GLOBAL_LONG);
    uint8_t set_op = (arg != -1) ? OP_SET_LOCAL : (arg < 256 ? OP_SET_GLOBAL : OP_SET_GLOBAL_LONG);

    if (arg == -1) arg = identifier_constant(&name);    // Global variable.

    if (can_assign && match(TOKEN_EQUAL)) {
        expression();
        if (get_op == OP_SET_GLOBAL_LONG) {
            emit_byte(set_op);
            emit_constant_24bit(arg);
        } else {
            emit_bytes(set_op, arg, -1);
        }
    } else {
        if (get_op == OP_GET_GLOBAL_LONG) {
            emit_byte(get_op);
            emit_constant_24bit(arg);
        } else {
            emit_bytes(get_op, arg, -1);
        }
    }
}

/* variable: function for resolving variables. */
static void variable(bool can_assign)
{
    named_variable(parser.previous, can_assign);
}

/* unary: function for compiling unary expressions. */
static void unary(bool can_assign)
{
    TokenType operator_type = parser.previous.type;
    parse_precedence(PREC_UNARY);

    switch (operator_type) {
        case TOKEN_BANG: emit_byte(OP_NOT); break;
        case TOKEN_MINUS: emit_byte(OP_NEGATE); break;
        default: return;
    }
}

/* Table of function pointers. */
ParseRule rules[] = {     /* prefix    infix      precedence */
    [TOKEN_LEFT_PAREN]    = {grouping, NULL,      PREC_NONE},
    [TOKEN_RIGHT_PAREN]   = {NULL,     NULL,      PREC_NONE},
    [TOKEN_LEFT_BRACE]    = {NULL,     NULL,      PREC_NONE},
    [TOKEN_RIGHT_BRACE]   = {NULL,     NULL,      PREC_NONE},
    [TOKEN_COMMA]         = {NULL,     NULL,      PREC_NONE},
    [TOKEN_DOT]           = {NULL,     NULL,      PREC_NONE},
    [TOKEN_MINUS]         = {unary,    binary,    PREC_TERM},
    [TOKEN_PLUS]          = {NULL,     binary,    PREC_TERM},
    [TOKEN_SEMICOLON]     = {NULL,     NULL,      PREC_NONE},
    [TOKEN_SLASH]         = {NULL,     binary,    PREC_FACTOR},
    [TOKEN_STAR]          = {NULL,     binary,    PREC_FACTOR},
    [TOKEN_BANG]          = {unary,    NULL,      PREC_NONE},
    [TOKEN_BANG_EQUAL]    = {NULL,     binary,    PREC_EQUALITY},
    [TOKEN_EQUAL]         = {NULL,     NULL,      PREC_NONE},
    [TOKEN_QUESTION]      = {NULL,     ternary,   PREC_TERNARY},
    [TOKEN_COLON]         = {NULL,     NULL,      PREC_NONE},
    [TOKEN_EQUAL_EQUAL]   = {NULL,     binary,    PREC_EQUALITY},
    [TOKEN_GREATER]       = {NULL,     binary,    PREC_COMPARISON},
    [TOKEN_GREATER_EQUAL] = {NULL,     binary,    PREC_COMPARISON},
    [TOKEN_LESS]          = {NULL,     binary,    PREC_COMPARISON},
    [TOKEN_LESS_EQUAL]    = {NULL,     binary,    PREC_COMPARISON},
    [TOKEN_IDENTIFIER]    = {variable, NULL,      PREC_NONE},
    [TOKEN_STRING]        = {string,   NULL,      PREC_NONE},
    [TOKEN_NUMBER]        = {number,   NULL,      PREC_NONE},
    [TOKEN_AND]           = {NULL,     logic_and, PREC_AND},
    [TOKEN_CLASS]         = {NULL,     NULL,      PREC_NONE},
    [TOKEN_ELSE]          = {NULL,     NULL,      PREC_NONE},
    [TOKEN_FALSE]         = {literal,  NULL,      PREC_NONE},
    [TOKEN_FOR]           = {NULL,     NULL,      PREC_NONE},
    [TOKEN_FUN]           = {NULL,     NULL,      PREC_NONE},
    [TOKEN_IF]            = {NULL,     NULL,      PREC_NONE},
    [TOKEN_NIL]           = {literal,  NULL,      PREC_NONE},
    [TOKEN_OR]            = {NULL,     logic_or,  PREC_OR},
    [TOKEN_PRINT]         = {NULL,     NULL,      PREC_NONE},
    [TOKEN_RETURN]        = {NULL,     NULL,      PREC_NONE},
    [TOKEN_THIS]          = {NULL,     NULL,      PREC_NONE},
    [TOKEN_TRUE]          = {literal,  NULL,      PREC_NONE},
    [TOKEN_VAR]           = {NULL,     NULL,      PREC_NONE},
    [TOKEN_WHILE]         = {NULL,     NULL,      PREC_NONE},
    [TOKEN_ERROR]         = {NULL,     NULL,      PREC_NONE},
    [TOKEN_EOF]           = {NULL,     NULL,      PREC_NONE},
};

/* get_rule: returns the rule at a given index. */
static ParseRule *get_rule(TokenType type)
{
    return &rules[type];
}

/* expression: expression → assignment ; */
static void expression()
{
    parse_precedence(PREC_ASSIGNMENT);
}

/* block: block → "{" declaration* "}" ; */
static void block()
{
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF))
        declaration();

    consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

/* var_declaration: varDecl → "var" IDENTIFIER ( "=" expression )? ";" ; */
static void var_declaration()
{
    int global = parse_variable("Expect variable name.");

    if (match(TOKEN_EQUAL))
        expression();
    else
        emit_byte(OP_NIL);

    consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");

    define_variable(global);
}

/* expression_statement: exprStmt → expression ";" ; */
static void expression_statement()
{
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
    emit_byte(OP_POP);
}

/* for_statement: forStmt  → "for" "(" ( varDecl | exprStmt | ";" )
                           expression? ";"
                           expression? ")" statement ; */
static void for_statement()
{
    begin_scope();
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");
    if (match(TOKEN_SEMICOLON));
        // No initializer.
    else if (match(TOKEN_VAR))
        var_declaration();
    else
        expression_statement();

    int loop_start = current_chunk()->count;
    int exit_jump = -1;
    if (!match(TOKEN_SEMICOLON)) {
        expression();
        consume(TOKEN_SEMICOLON, "Expect ';' after loop condition.");

        // Jump out of the loop if the condition is false.
        exit_jump = emit_jump(OP_JUMP_IF_FALSE);
        emit_byte(OP_POP); // Condition.
    }

    if (!match(TOKEN_RIGHT_PAREN)) {
        int body_jump = emit_jump(OP_JUMP);
        int increment_start = current_chunk()->count;
        expression();
        emit_byte(OP_POP);
        consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

        emit_loop(loop_start);
        loop_start = increment_start;
        patch_jump(body_jump);
    }

    // For continue statement to jump to start of increment if present.
    current_continue_jump = loop_start;

    statement();
    emit_loop(loop_start);

    if (exit_jump != -1) {
        patch_jump(exit_jump);
        emit_byte(OP_POP); // Condition.
    }

    end_scope();
}

/* if_statement: ifStmt → "if" "(" expression ")" statement
                 ( "else" statement )? ; */
static void if_statement()
{
    // Condition expression leaves its value on the stack.
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    int then_jump = emit_jump(OP_JUMP_IF_FALSE);
    emit_byte(OP_POP);  // Pop the condition expression's value from the stack.
    statement();        // Then statement.

    int else_jump = emit_jump(OP_JUMP);

    patch_jump(then_jump);
    emit_byte(OP_POP);  // Pop the condition expression's value from the stack.

    if (match(TOKEN_ELSE)) statement();    // Else statement.
    patch_jump(else_jump);
}

/* print_statement: printStmt → "print" expression ";" ;  */
static void print_statement()
{
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after value.");
    emit_byte(OP_PRINT);
}

/* while_statement: whileStmt → "while" "(" expression ")" statement ; */
static void while_statement()
{
    int loop_start = current_chunk()->count;

    // For continue statement to jump to beginning of loop.
    current_continue_jump = loop_start;

    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    int exit_jump = emit_jump(OP_JUMP_IF_FALSE);
    emit_byte(OP_POP);  // Condition.

    statement();

    emit_loop(loop_start);
    patch_jump(exit_jump);
    emit_byte(OP_POP);  // Condition.
}

/* continue_statement: continueStmt → "continue" ";" ; */
static void continue_statement()
{
    if (current_continue_jump != -1)
        emit_loop(current_continue_jump);
    else
        error("`continue` statement not within a loop.");
    consume(TOKEN_SEMICOLON, "Expect ';' after 'continue'.");
}

/* switch_statement: switchStmt  → "switch" "(" expression ")"
                                  "{" switchCase* defaultCase? "}" ;
                     switchCase  → "case" expression ":" statement* ;
                     defaultCase → "default" ":" statement* ; */
static void switch_statement()
{
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'switch'.");
    expression();   // switch expr - leaves its value on stack.
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");
    consume(TOKEN_LEFT_BRACE, "Expect '{' before case(s).");

    int case_jump_list[MAX_CASES];
    int case_jump_count = 0;

    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
        if (match(TOKEN_CASE)) {
            expression();

            int next_jump = emit_jump(OP_JUMP_NOT_EQUAL);   // jump to next case.
            emit_byte(OP_POP);

            consume(TOKEN_COLON, "Expect ':' after case expression.");
            statement();    // execute case statement if its expr == switch expr.

            int end_jump = emit_jump(OP_JUMP);
            patch_jump(next_jump);

            case_jump_list[case_jump_count++] = end_jump;
            if (case_jump_count == MAX_CASES) error("Too many cases in switch statement.");
        }

        if (match(TOKEN_DEFAULT)) {
            consume(TOKEN_COLON, "Expect ':' after default.");
            statement();
        }
    }

    // Patch all jumps to go to the end of the switch statement.
    for (int i = 0; i < case_jump_count; i++)
        patch_jump(case_jump_list[i]);

    consume(TOKEN_RIGHT_BRACE, "Expect '}' after switch-case statement.");
    emit_byte(OP_POP);
}

/* teddy_statement: teddyStmt → "teddy" ";" ; */
static void teddy_statement()
{
    consume(TOKEN_SEMICOLON, "Expect ';' after teddy statement.");
    emit_byte(OP_TEDDY);
}

/* synchronize: when in panic mode, skip tokens until statment boundary. */
static void synchronize()
{
    parser.panic_mode = false;

    while (parser.current.type != TOKEN_EOF) {
        if (parser.previous.type == TOKEN_SEMICOLON) return;
        switch (parser.current.type) {
            case TOKEN_CLASS:
            case TOKEN_FUN:
            case TOKEN_VAR:
            case TOKEN_FOR:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_PRINT:
            case TOKEN_RETURN:
            return;

            default:
                ; // Do nothing.
        }
        advance();
    }
}

/* declaration: declaration → classDecl | funDecl | varDecl | statement ;  */
static void declaration()
{
    if (match(TOKEN_VAR))
        var_declaration();
    else
        statement();

    if (parser.panic_mode) synchronize();
}

/* statement: statement → exprStmt | forStmt | ifStmt | printStmt | returnStmt
                          | whileStmt | switchStmt | block ; */
static void statement()
{
    if (match(TOKEN_PRINT)) {
        print_statement();
    } else if (match(TOKEN_FOR)) {
        for_statement();
    } else if (match(TOKEN_IF)) {
        if_statement();
    } else if(match(TOKEN_WHILE)) {
        while_statement();
    } else if (match(TOKEN_SWITCH)) {
        switch_statement();
    } else if (match(TOKEN_CONTINUE)) {
        continue_statement();
    } else if (match(TOKEN_TEDDY)) {
        teddy_statement();
    } else if (match(TOKEN_LEFT_BRACE)) {
        begin_scope();
        block();
        end_scope();
    } else {
        expression_statement();
    }
}

/* compile: compile the source text. */
bool compile(const char *source, Chunk *chunk)
{
    init_scanner(source);
    Compiler compiler;
    init_compiler(&compiler);
    compiling_chunk = chunk;

    parser.had_error = false;
    parser.panic_mode = false;

    advance();

    while (!match(TOKEN_EOF))
        declaration();

    end_compiler();
    return !parser.had_error;
}
