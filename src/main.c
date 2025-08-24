#include "ast.c"
#include "codegen.c"
#include "lexer.c"
#include "parser.c"
#include <stdio.h>
#include <stdlib.h>

// Helper function to read a file's contents into a string.
char *get_file_contents(const char *filepath) {
  // Opening the file in binary read mode.
  FILE *file = fopen(filepath, "rb");

  // Checking if the file was opened successfully.
  if (!file) {
    fprintf(stderr, "Could not open file at: %s\n", filepath);
    exit(1);
  }

  // Go to the end of the file to determine its size.
  fseek(file, 0L, SEEK_END);
  size_t size = ftell(file);
  rewind(file); // Go back to the start.

  // Allocating memory to hold the contents of the opened file.
  char *buffer = (char *)malloc(size + 1);
  if (!buffer) {
    fprintf(stderr, "Failed to allocate memory to load the file.\n");
    fclose(file);
    exit(1);
  }

  // Reading the file into the memory and null-terminating the buffer.
  size_t bytes_read = fread(buffer, 1, size, file);
  if (bytes_read < size) {
    fprintf(stderr, "Failed to read the entire file.\n");
    free(buffer);
    fclose(file);
    exit(1);
  }
  buffer[size] = '\0';

  // Close the file.
  fclose(file);

  return buffer;
}

int main() {
  // FUTURE: Update this to read the actual file content.
  char *source_code = get_file_contents("./testing/main.fl");

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
  ast_free(translation_unit);
  puts(ir);

  return 0;
}