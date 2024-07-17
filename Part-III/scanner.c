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

/* is_alpha: returns true if char is a letter or underscore. */
static bool is_alpha(char c)
{
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           c == '_';
}

/* is_digit: returns true if char is a digit. */
static bool is_digit(char c)
{
    return c >= '0' && c <= '9';
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

/* skip_block_comment: skips over C style block comments (like this one). */
static void skip_block_comment() {
    advance(); // Advance past '/'
    advance(); // Advance past '*'

    while (true) {
        if (peek() == '\n') {
            scanner.line++;
        } else if (peek() == '*' && peek_next() == '/') {
            advance();
            advance();
            break;
        } else if (is_at_end()) {
            error_token("Unterminated block comment.");
            return;
        }
        advance();
    }
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
                    skip_block_comment();
                } else {
                    return;
                }
                break;
            default:
                return;
        }
    }
}

/* check_keyword: tests the rest of a potential keyword's lexeme. */
static TokenType check_keyword(int start, int length, const char *rest, TokenType type)
{
    if (scanner.current - scanner.start == start + length &&
            memcmp(scanner.start + start, rest, length) == 0)
        return type;

    return TOKEN_IDENTIFIER;
}

/* identifier_type: return proper identifier token type. */
static TokenType identifier_type()
{
    switch (scanner.start[0]) {
        // Initial letters that correspond to a single keyword.
        case 'a': return check_keyword(1, 2, "nd", TOKEN_AND);
        case 'd': return check_keyword(1, 6, "efault", TOKEN_DEFAULT);
        case 'e': return check_keyword(1, 3, "lse", TOKEN_ELSE);
        case 'i': return check_keyword(1, 1, "f", TOKEN_IF);
        case 'n': return check_keyword(1, 2, "il", TOKEN_NIL);
        case 'o': return check_keyword(1, 1, "r", TOKEN_OR);
        case 'p': return check_keyword(1, 4, "rint", TOKEN_PRINT);
        case 'r': return check_keyword(1, 5, "eturn", TOKEN_RETURN);
        case 'v': return check_keyword(1, 2, "ar", TOKEN_VAR);
        case 'w': return check_keyword(1, 4, "hile", TOKEN_WHILE);

        // Initial letters that correspond to several keywords.
        case 'c':
            if (scanner.current - scanner.start > 1)
                switch (scanner.start[1]) {
                    case 'a': return check_keyword(2, 2, "se", TOKEN_CASE);
                    case 'l': return check_keyword(2, 3, "ass", TOKEN_CLASS);
                    case 'o': return check_keyword(2, 6, "ntinue", TOKEN_CONTINUE);
                }
            break;
        case 'f':
            if (scanner.current - scanner.start > 1)
                switch (scanner.start[1]) {
                    case 'a': return check_keyword(2, 3, "lse", TOKEN_FALSE);
                    case 'o': return check_keyword(2, 1, "r", TOKEN_FOR);
                    case 'u': return check_keyword(2, 1, "n", TOKEN_FUN);
                }
            break;
        case 's':
            if (scanner.current - scanner.start > 1)
                switch (scanner.start[1]) {
                    case 'u': return check_keyword(2, 3, "per", TOKEN_SUPER);
                    case 'w': return check_keyword(2, 4, "itch", TOKEN_SWITCH);
                }
            break;
        case 't':
            if (scanner.current - scanner.start > 1)
                switch (scanner.start[1]) {
                    case 'h': return check_keyword(2, 2, "is", TOKEN_THIS);
                    case 'r': return check_keyword(2, 2, "ue", TOKEN_TRUE);
                }
            break;
    }
    return TOKEN_IDENTIFIER;
}

/* identifier: handle identifier tokens. */
static Token identifier()
{
    while (is_alpha(peek()) || is_digit(peek())) advance();
    return make_token(identifier_type());
}

/* number: handles number literals. */
static Token number()
{
    while (is_digit(peek())) advance();

    // Look for a fractional part.
    if (peek() == '.' && is_digit(peek_next())) {
        advance();

        while (is_digit(peek())) advance();
    }

    return make_token(TOKEN_NUMBER);
}

/* string: handles string literals. */
static Token string()
{
    while (peek() != '"' && !is_at_end()) {
        if (peek() == '\n') scanner.line++;
        advance();
    }

    if (is_at_end()) return error_token("Unterminated string.");

    advance();
    return make_token(TOKEN_STRING);
}

/* scan_token: scans a complete token. */
Token scan_token()
{
    skip_whitespace();
    scanner.start = scanner.current;

    if (is_at_end()) return make_token(TOKEN_EOF);

    char c = advance();

    if (is_alpha(c)) return identifier();
    if (is_digit(c)) return number();

    switch (c) {
        case '(': return make_token(TOKEN_LEFT_PAREN);
        case ')': return make_token(TOKEN_RIGHT_PAREN);
        case '{': return make_token(TOKEN_LEFT_BRACE);
        case '}': return make_token(TOKEN_RIGHT_BRACE);
        case ';': return make_token(TOKEN_SEMICOLON);
        case ',': return make_token(TOKEN_COMMA);
        case '.': return make_token(TOKEN_DOT);
        case '/': return make_token(TOKEN_SLASH);
        case '*': return make_token(TOKEN_STAR);
        case '+': return make_token(TOKEN_PLUS);
        case '-': return make_token(TOKEN_MINUS);
        case '?': return make_token(TOKEN_QUESTION);
        case ':': return make_token(TOKEN_COLON);
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
        case '"': return string(); break;
    }

    return error_token("Unexpected character.");

}
