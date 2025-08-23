#include "ast.c"
#include "codegen.c"
#include "lexer.c"
#include "parser.c"
#include <stdio.h>
#include <stdlib.h>

int main() {
  // FUTURE: Update this to read the actual file content.
  const char *source_code = "int main() {return 0;}\0";

  // Initalising the lexer.
  Lexer *lexer = (Lexer *)(malloc(sizeof(Lexer)));
  lexer_init(lexer, source_code);

  // Initalising the parser.
  Parser *parser = (Parser *)(malloc(sizeof(Parser)));
  parser_init(parser, lexer);

  // Generating a translation unit
  AstNode *translation_unit = parse_translation_unit(parser);

  // Printing the program to the console.
  const char *ir = codegen(translation_unit);
  puts(ir);

  return 0;
}