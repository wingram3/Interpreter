#include <stdio.h>
#include <string.h>

#include "common.h"
#include "scanner.h"

/* Scanner structure. */
typedef struct {
    const char *start;      // marks start of current lexeme being scanned.
    const char *current;    // points to the current char being looked at.
    int line;               // tracks the current source line for error reporting.
} Scanner;

Scanner scanner;

/* init_scanner: initialize the scanner. */
void init_scanner(const char *source)
{
    scanner.start = source;
    scanner.current = source;
    scanner.line = 1;
}

/* is_at_end: returns true if at EOF. */
static bool is_at_end()
{
    return *scanner.current == '\0';
}

/* advance: consumes the current char and returns it. */
static char advance()
{
    scanner.current++;
    return scanner.current[-1];
}

/* peek: returns the current char but does not consume it. */
static char peek()
{
    return *scanner.current;
}

/* peek_next: a second character of lookahead. */
static char peek_next()
{
    if (is_at_end()) return '\0';
    return scanner.current[1];
}

/* match: consume a char if it matches an expected char. */
static bool match(char expected)
{
    if (is_at_end()) return false;
    if (*scanner.current != expected) return false;
    scanner.current++;
    return true;
}

/* make_token: constructor-like function to create a token. */
static Token make_token(TokenType type)
{
    Token token;
    token.type = type;
    token.start = scanner.start;
    token.length = (int)(scanner.current - scanner.start);
    token.line = scanner.line;
    return token;
}

/* error_token: sister function to make_token for error tokens. */
static Token error_token(const char *message)
{
    Token token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = (int)strlen(message);
    token.line = scanner.line;
    return token;
}

/* skip_comment: advances scanner past block comments like this one. */
static Token skip_comment()
{
    while(peek() != '*' && !is_at_end()) {
        if (peek() == '\n') scanner.line++;
        advance();
    }
    if (is_at_end()) return error_token("Unterminated block comment.");
    advance();

    if (peek() != '/') {
        return error_token("Unterminated block comment");
    }

    advance();
    return make_token(TOKEN_BLOCK_COMMENT);
}

/* skip_whitespace: advances scanner past any leading whitespace. */
static void skip_whitespace()
{
    for (;;) {
        char c = peek();
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance();
                break;
            case '\n':
                scanner.line++;
                advance();
                break;
            case '/':
                if (peek_next() == '/') {
                    while (peek() != '\n' && !is_at_end()) advance();
                } else if (peek_next() == '*') {
                    skip_comment();
                } else {
                    return;
                }
                break;
            default:
                return;
        }
    }
}

/* scan_token: scans a complete token. */
Token scan_token()
{
    skip_whitespace();
    scanner.start = scanner.current;

    if (is_at_end()) return make_token(TOKEN_EOF);

    char c = advance();

    switch (c) {
        case '(': return make_token(TOKEN_LEFT_PAREN);
        case ')': return make_token(TOKEN_RIGHT_PAREN);
        case '{': return make_token(TOKEN_LEFT_BRACE);
        case '}': return make_token(TOKEN_RIGHT_BRACE);
        case ';': return make_token(TOKEN_SEMICOLON);
        case ',': return make_token(TOKEN_COMMA);
        case '.': return make_token(TOKEN_DOT);
        case '-': return make_token(TOKEN_MINUS);
        case '+': return make_token(TOKEN_PLUS);
        case '*': return make_token(TOKEN_STAR);
        case '!':
            return make_token(
                match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
        case '=':
            return make_token(
                match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
        case '<':
            return make_token(
                match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
        case '>':
            return make_token(
                match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
    }

    return error_token("Unexpected character.");

}
