#include "include/lexer.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

typedef struct {
  const char *name;
  TokenKind kind;
} SpecialWord;

SpecialWord special_words[] = {{"int", TOKEN_INT},
                               {"return", TOKEN_RETURN},
                               {"String", TOKEN_STRING},
                               {"@foreign", TOKEN_FOREIGN},
                               {"void", TOKEN_VOID}};

// Function to intialise the lexer.
void lexer_init(Lexer *lexer, const char *source_code) {
  // Setting the fields of the lexer.
  lexer->line = 1;
  lexer->column = 0;
  lexer->start_ptr = source_code;
  lexer->current_ptr = source_code;
}

// Function to convert the token kind to string.
const char *token_kind_to_string(TokenKind token_kind) {
  switch (token_kind) {
  case TOKEN_VOID:
    return "TOKEN_VOID";
  case TOKEN_INT:
    return "TOKEN_INT";
  case TOKEN_STRING:
    return "TOKEN_STRING";
  case TOKEN_FOREIGN:
    return "TOKEN_FOREIGN";
  case TOKEN_STRING_LITERAL:
    return "TOKEN_STRING_LITERAL";
  case TOKEN_RETURN:
    return "TOKEN_RETURN";
  case TOKEN_INT_LITERAL:
    return "TOKEN_INT_LITERAL";
  case TOKEN_LPAREN:
    return "TOKEN_LPAREN";
  case TOKEN_RPAREN:
    return "TOKEN_RPAREN";
  case TOKEN_LBRACE:
    return "TOKEN_LBRACE";
  case TOKEN_RBRACE:
    return "TOKEN_RBRACE";
  case TOKEN_SEMICOLON:
    return "TOKEN_SEMICOLON";
  case TOKEN_COMMA:
    return "TOKEN_COMMA";
  case TOKEN_IDENTIFIER:
    return "TOKEN_IDENTIFIER";
  case TOKEN_EOF:
    return "TOKEN_EOF";
  }

  return "TOKEN_?";
}

// Helper function to obtain the current character.
char peek(Lexer *lexer) {
  // Returning the the current character.
  return *lexer->current_ptr;
}

// Helper function to advance the lexer.
char advance(Lexer *lexer) {
  char previous_character = *lexer->current_ptr;
  lexer->current_ptr++;
  return previous_character;
}

// Helper function to skip over whitepaces and comments.
void skip_whitespaces_and_comments(Lexer *lexer) {
  // Running the loop indefinately intentionally.
  for (;;) {
    // Lookup the current character.
    switch (peek(lexer)) {
    case ' ':
    case '\r':
    case '\t':
      advance(lexer);
      lexer->column++;
      break;

    case '\n':
      advance(lexer);
      lexer->line++;
      lexer->column = 0;
      break;

    case '#':
      do {
        advance(lexer);
        lexer->column++;
      } while (peek(lexer) != '\n' && peek(lexer) != '\0');
      break;

    default:
      // If the program execution reaches here,
      // therefore, none of the above case statements were matched.
      // Hence, we break the for loop.
      return;
    }
  }
}

// Helper function to make a token.
Token make_token(Lexer *lexer, TokenKind token_kind) {
  Token token = {
      .kind = token_kind,
      .line = lexer->line,
      .start_ptr = lexer->start_ptr,
      .length = (size_t)(lexer->current_ptr - lexer->start_ptr),
  };

  return token;
}

// Helper function to check if the identifier is a special word.
TokenKind is_special_word(const char *word, size_t token_length) {
  for (size_t i = 0; i < sizeof(special_words) / sizeof(SpecialWord); i++) {
    if (strlen(special_words[i].name) == token_length &&
        strncmp(word, special_words[i].name, token_length) == 0) {
      return special_words[i].kind;
    }
  }

  return TOKEN_IDENTIFIER;
}

// Helper function to make special word.
Token make_special_word(Lexer *lexer) {
  // Advance the lexer until the end of word.
  while (isalpha(peek(lexer)) || isdigit(peek(lexer)) || peek(lexer) == '_') {
    advance(lexer);
  }

  size_t token_length = (size_t)(lexer->current_ptr - lexer->start_ptr);
  for (size_t i = 0; i < sizeof(special_words) / sizeof(SpecialWord); i++) {
    if (strlen(special_words[i].name) == token_length &&
        strncmp(lexer->start_ptr, special_words[i].name, token_length) == 0) {
      return make_token(lexer, special_words[i].kind);
    }
  }

  // Error Message
  fprintf(stderr, "Lexer error: Unrecognized special word '%.*s' at line %zu\n",
          (int)token_length, lexer->start_ptr, lexer->line);
  exit(1);
}

// Helper function to generate identifiers.
Token make_identifier_token(Lexer *lexer) {
  // Advance the lexer until the end of word.
  while (isalpha(peek(lexer)) || isdigit(peek(lexer)) || peek(lexer) == '_') {
    advance(lexer);
  }

  return make_token(
      lexer, is_special_word(lexer->start_ptr,
                             (size_t)(lexer->current_ptr - lexer->start_ptr)));
}

// Helper function to generate numbers.
Token make_number_token(Lexer *lexer) {
  while (isdigit(peek(lexer))) {
    advance(lexer);
  }

  return make_token(lexer, TOKEN_INT_LITERAL);
}

Token make_string_token(Lexer *lexer) {
  // Consume until closing quote or EOF
  while (peek(lexer) != '"' && peek(lexer) != '\0') {
    if (peek(lexer) == '\\') {
      advance(lexer);
      advance(lexer);
    } else {
      advance(lexer);
    }
  }

  if (peek(lexer) != '"') {
    fprintf(stderr, "Unterminated string at line %zu\n", lexer->line);
    exit(1);
  }

  advance(lexer);
  return make_token(lexer, TOKEN_STRING_LITERAL);
}

// Function to compute next token.
Token compute_next_token(Lexer *lexer) {
  // Skipping whitespaces and comments.
  skip_whitespaces_and_comments(lexer);

  // Setting the starting position in the lexer for this token.
  lexer->start_ptr = lexer->current_ptr;

  // Getting the character from the lexer.
  char previous_character = advance(lexer);

  if (previous_character == '"') {
    return make_string_token(lexer);
  }

  if (previous_character == '@') {
    return make_special_word(lexer);
  }

  if (isdigit(previous_character)) {
    return make_number_token(lexer);
  }
  if (isalpha(previous_character) || previous_character == '_') {
    return make_identifier_token(lexer);
  }

  switch (previous_character) {
  case '\0':
    return make_token(lexer, TOKEN_EOF);
  case '(':
    return make_token(lexer, TOKEN_LPAREN);
  case ')':
    return make_token(lexer, TOKEN_RPAREN);
  case '{':
    return make_token(lexer, TOKEN_LBRACE);
  case '}':
    return make_token(lexer, TOKEN_RBRACE);
  case ';':
    return make_token(lexer, TOKEN_SEMICOLON);
  case ',':
    return make_token(lexer, TOKEN_COMMA);
  }

  exit(1);
};