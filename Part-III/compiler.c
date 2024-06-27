#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "common.h"
#include "compiler.h"
#include "scanner.h"

typedef struct {
    Token current;
    Token previous;
    bool had_error;
    bool panic_mode;
} Parser;

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

/* end_compiler:  */
static void end_compiler()
{
    emit_return();
}

/* compile: compile the source text. */
bool compile(const char *source, Chunk *chunk)
{
    init_scanner(source);
    compiling_chunk = chunk;

    parser.had_error = false;
    parser.panic_mode = false;

    advance();
    // expression();
    consume(TOKEN_EOF, "Expect end of expression.");
    end_compiler();
    return !parser.had_error;
}
