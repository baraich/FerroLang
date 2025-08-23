#ifndef FERRO_LANG_PARSER
#define FERRO_LANG_PARSER

#include "ast.h"
#include "lexer.h"

// Parser defination
typedef struct {
  Lexer *lexer;
  Token current_token;
  Token previous_token;
} Parser;

// Initalise the parser.
void parser_init(Parser *parser, Lexer *lexer);

// Generate a translation unit.
AstNode *parse_translation_unit(Parser *parser);

#endif