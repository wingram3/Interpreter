#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "chunk.h"
#include "common.h"
#include "compiler.h"
#include "scanner.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

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

typedef void (*ParseFn)();

/* Structure to represent a single row in the parser table. */
typedef struct {
    ParseFn prefix;
    ParseFn infix;
    ParseFn mixfix;
    Precedence precedence;
} ParseRule;

Parser parser;
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

/* emit_byte: append a single byte to the chunk. */
static void emit_byte(uint8_t byte)
{
    write_chunk(current_chunk(), byte, parser.previous.line);
}

/* emit_bytes: append an arbitary number of bytes to the chunk.
   usage: emit_bytes(byte1, byte2, byte3, ..., -1); */
static void emit_bytes(int first_byte, ...)
{
    va_list args;
    va_start(args, first_byte);
    emit_byte((uint8_t)first_byte);

    int byte;
    while ((byte = va_arg(args, int)) != -1)  // -1 is the sentinel value.
        emit_byte((uint8_t)byte);

    va_end(args);
}

/* emit_return:  */
static void emit_return()
{
    emit_byte(OP_RETURN);
}

/* make_constant: insert an entry into the constant pool. */
static int make_constant(Value value)
{
    int constant = add_constant(current_chunk(), value);
    if (constant > 16777216) {
        error("Too many constants in one chunk.");
        return 0;
    }

    return constant;
}

/* emit_constant: emit a constant value. */
static void emit_constant(Value value)
{
    int constant = make_constant(value);
    if (constant < 256) {
        emit_bytes(OP_CONSTANT, (uint8_t)constant, -1);
    } else {    // write the constant index as a 24-bit number if the index > 255.
        emit_bytes(OP_CONSTANT_LONG, (uint8_t)(constant & 0xFF),
            (uint8_t)((constant >> 8) & 0xFF),
            (uint8_t)((constant >> 16) & 0xFF), -1);
    }
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

static void expression();
static ParseRule *get_rule(TokenType type);
static void parse_precedence(Precedence precedence);

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

    prefix_rule();

    while (precedence <= get_rule(parser.current.type)->precedence) {
        advance();
        ParseFn infix_rule = get_rule(parser.previous.type)->infix;
        infix_rule();
    }
}

/* binary: function for compiling binary expressions. */
static void binary()
{
    TokenType operator_type = parser.previous.type;
    ParseRule *rule = get_rule(operator_type);
    parse_precedence((Precedence)(rule->precedence + 1));

    switch (operator_type) {
        case TOKEN_BANG_EQUAL:    emit_bytes(OP_EQUAL, OP_NOT); break;
        case TOKEN_EQUAL_EQUAL:   emit_byte(OP_EQUAL); break;
        case TOKEN_GREATER:       emit_byte(OP_GREATER); break;
        case TOKEN_GREATER_EQUAL: emit_bytes(OP_LESS, OP_NOT); break;
        case TOKEN_LESS:          emit_byte(OP_LESS); break;
        case TOKEN_LESS_EQUAL:    emit_bytes(OP_GREATER, OP_NOT); break;
        case TOKEN_PLUS:          emit_byte(OP_ADD); break;
        case TOKEN_MINUS:         emit_byte(OP_SUBTRACT); break;
        case TOKEN_STAR:          emit_byte(OP_MULTIPLY); break;
        case TOKEN_SLASH:         emit_byte(OP_DIVIDE); break;
        default:
            return;
    }
}

/* literal: function for compiling true, false, and nil. */
static void literal()
{
    switch (parser.previous.type) {
        case TOKEN_FALSE: emit_byte(OP_FALSE); break;
        case TOKEN_NIL: emit_byte(OP_NIL); break;
        case TOKEN_TRUE: emit_byte(OP_TRUE); break;
        default: return;
    }
}

/* ternary: function for compiling ternary (conditional) expressions. */
static void ternary()
{
    // implement with other jump codes.
}

/* grouping: function for compiling grouping expressions. */
static void grouping()
{
    expression();   // this inner call handles bytecode generation for the expr in parentheses.
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

/* number: function for compiling number literal expressions. */
static void number()
{
    double value = strtod(parser.previous.start, NULL);
    emit_constant(NUMBER_VAL(value));
}

/* unary: function for compiling unary expressions. */
static void unary()
{
    TokenType operator_type = parser.previous.type;

    parse_precedence(PREC_UNARY);   // compile the operand.

    // Emit the operator expression.
    switch (operator_type) {
        case TOKEN_BANG: emit_byte(OP_NOT); break;
        case TOKEN_MINUS: emit_byte(OP_NEGATE); break;
        default: return;
    }
}

/* Table of function pointers. */
ParseRule rules[] = {     /* prefix    infix     mixfix     precedence */
    [TOKEN_LEFT_PAREN]    = {grouping, NULL,     NULL,      PREC_NONE},
    [TOKEN_RIGHT_PAREN]   = {NULL,     NULL,     NULL,      PREC_NONE},
    [TOKEN_LEFT_BRACE]    = {NULL,     NULL,     NULL,      PREC_NONE},
    [TOKEN_RIGHT_BRACE]   = {NULL,     NULL,     NULL,      PREC_NONE},
    [TOKEN_COMMA]         = {NULL,     NULL,     NULL,      PREC_NONE},
    [TOKEN_DOT]           = {NULL,     NULL,     NULL,      PREC_NONE},
    [TOKEN_MINUS]         = {unary,    binary,   NULL,      PREC_TERM},
    [TOKEN_PLUS]          = {NULL,     binary,   NULL,      PREC_TERM},
    [TOKEN_SEMICOLON]     = {NULL,     NULL,     NULL,      PREC_NONE},
    [TOKEN_SLASH]         = {NULL,     binary,   NULL,      PREC_FACTOR},
    [TOKEN_STAR]          = {NULL,     binary,   NULL,      PREC_FACTOR},
    [TOKEN_BANG]          = {unary,    NULL,     NULL,      PREC_NONE},
    [TOKEN_BANG_EQUAL]    = {NULL,     binary,   NULL,      PREC_EQUALITY},
    [TOKEN_EQUAL]         = {NULL,     NULL,     NULL,      PREC_NONE},
    [TOKEN_QUESTION]      = {NULL,     NULL,     ternary,   PREC_TERNARY},
    [TOKEN_COLON]         = {NULL,     NULL,     NULL,      PREC_TERNARY},
    [TOKEN_EQUAL_EQUAL]   = {NULL,     binary,     NULL,      PREC_NONE},
    [TOKEN_GREATER]       = {NULL,     binary,     NULL,      PREC_NONE},
    [TOKEN_GREATER_EQUAL] = {NULL,     binary,     NULL,      PREC_NONE},
    [TOKEN_LESS]          = {NULL,     binary,     NULL,      PREC_NONE},
    [TOKEN_LESS_EQUAL]    = {NULL,     binary,     NULL,      PREC_NONE},
    [TOKEN_NUMBER]        = {number,   NULL,     NULL,      PREC_NONE},
    [TOKEN_CLASS]         = {NULL,     NULL,     NULL,      PREC_NONE},
    [TOKEN_ELSE]          = {NULL,     NULL,     NULL,      PREC_NONE},
    [TOKEN_FALSE]         = {literal,  NULL,     NULL,      PREC_NONE},
    [TOKEN_FOR]           = {NULL,     NULL,     NULL,      PREC_NONE},
    [TOKEN_FUN]           = {NULL,     NULL,     NULL,      PREC_NONE},
    [TOKEN_IF]            = {NULL,     NULL,     NULL,      PREC_NONE},
    [TOKEN_NIL]           = {literal,  NULL,     NULL,      PREC_NONE},
    [TOKEN_PRINT]         = {NULL,     NULL,     NULL,      PREC_NONE},
    [TOKEN_RETURN]        = {NULL,     NULL,     NULL,      PREC_NONE},
    [TOKEN_THIS]          = {NULL,     NULL,     NULL,      PREC_NONE},
    [TOKEN_TRUE]          = {literal,  NULL,     NULL,      PREC_NONE},
    [TOKEN_VAR]           = {NULL,     NULL,     NULL,      PREC_NONE},
    [TOKEN_WHILE]         = {NULL,     NULL,     NULL,      PREC_NONE},
    [TOKEN_ERROR]         = {NULL,     NULL,     NULL,      PREC_NONE},
    [TOKEN_EOF]           = {NULL,     NULL,     NULL,      PREC_NONE},
};

/* get_rule: returns the rule at a given index. */
static ParseRule *get_rule(TokenType type)
{
    return &rules[type];
}

/* expression:  */
static void expression()
{
    parse_precedence(PREC_ASSIGNMENT);
}

/* compile: compile the source text. */
bool compile(const char *source, Chunk *chunk)
{
    init_scanner(source);
    compiling_chunk = chunk;

    parser.had_error = false;
    parser.panic_mode = false;

    advance();
    expression();
    consume(TOKEN_EOF, "Expect end of expression.");
    end_compiler();
    return !parser.had_error;
}
