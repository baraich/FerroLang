#ifndef FERRO_LANG_LEXER
#define FERRO_LANG_LEXER

#include <stdlib.h>

// Avaiable Token Possibilites.
typedef enum {
  TOKEN_INT,
  TOKEN_RETURN,

  // Literals
  TOKEN_IDENTIFIER,
  TOKEN_INT_LITERAL,

  // Symbols
  TOKEN_LPAREN,
  TOKEN_RPAREN,
  TOKEN_LBRACE,
  TOKEN_RBRACE,
  TOKEN_SEMICOLON,
  TOKEN_COMMA,

  // EOF
  TOKEN_EOF
} TokenKind;

// Token Defination
typedef struct {
  // Diagnostics Information
  size_t line;
  size_t length;

  TokenKind kind;
  const char *start_ptr;
} Token;

// Lexer Defination
// Responsible for generating appropriate tokens.
typedef struct {
  // Diagnostics Information
  size_t line;
  size_t column;

  const char *start_ptr;
  const char *current_ptr;
} Lexer;

// Function to intialise the lexer.
void lexer_init(Lexer *lexer, const char *source_code);

// Function to convert the token kind to string.
const char *token_kind_to_string(TokenKind token_kind);

// Function to get the next token.
Token compute_next_token(Lexer *lexer);

#endif