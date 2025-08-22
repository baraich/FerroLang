#include "include/lexer.h"
#include "lexer.c"
#include <stdio.h>
#include <stdlib.h>

int main() {
  // FUTURE: Update this to read the actual file content.
  const char *source_code = "int main() { return 0; }\0";

  // Initalising the lexer.
  Lexer *lexer = (Lexer *)(malloc(sizeof(Lexer)));
  init_lexer(lexer, source_code);

  // Printing tokens to the console.
  for (;;) {
    // Getting the next token from the lexer.
    Token token = compute_next_token(lexer);

    // Printing the token to the console.
    printf("%s\n", token_kind_to_string(token.kind));

    // Breaking the loop.
    if (token.kind == TOKEN_EOF) {
      break;
    }
  }

  return 0;
}