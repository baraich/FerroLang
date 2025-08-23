#include "include/parser.h"
#include "include/ast.h"
#include "include/helpers.h"
#include "include/lexer.h"
#include <stdbool.h>
#include <stdio.h>

// Helper function to advance the parser.
Token advance_parser(Parser *parser) {
  parser->previous_token = parser->current_token;
  parser->current_token = compute_next_token(parser->lexer);
  return parser->previous_token;
}

// Initalise the parser.
void parser_init(Parser *parser, Lexer *lexer) {
  parser->lexer = lexer;
  parser->current_token = (Token){0};
  parser->previous_token = (Token){0};

  // Setting the current token in the parser.
  advance_parser(parser);
}

// Helper function to check the current token.
bool check(Parser *parser, TokenKind expected_token_kind) {
  return parser->current_token.kind == expected_token_kind;
}

// Helper function to check if it's a primitive type.
bool is_primitive_type(TokenKind token_kind) {
  switch (token_kind) {
  case TOKEN_INT:
    return true;
  default:
    return false;
  }
}

// Helper function to advance the parser with expect.
Token advance_with_expect(Parser *parser, TokenKind expected_token_kind) {
  if (check(parser, expected_token_kind)) {
    return advance_parser(parser);
  };

  printf("Did not expect this token");
  exit(1);
}

// Helper function to parse expression.
AstNode *parse_expression(Parser *parser) {
  switch (parser->current_token.kind) {
  case TOKEN_INT_LITERAL: {
    Token t = advance_parser(parser);
    AstNode *node = ast_new(AST_INT_LITERAL_EXPRESSION, t);
    node->as.literal.token = t;
    return node;
  }
  default:
    break;
  }

  printf("%.*s", (int)parser->current_token.length,
         parser->current_token.start_ptr);
  exit(1);
}

// Helper function to parser return statement.
AstNode *parse_return_statement(Parser *parser) {
  AstNode *expression = parse_expression(parser);
  AstNode *node = ast_new(AST_RETURN_STATEMENT, parser->current_token);
  node->as.return_statement.value = expression;
  advance_with_expect(parser, TOKEN_SEMICOLON);

  return node;
}

// Helper function to parse statement.
AstNode *parse_statement(Parser *parser) {
  if (check(parser, TOKEN_RETURN)) {
    // Skip the return token.
    advance_parser(parser);
    return parse_return_statement(parser);
  }

  return parse_expression(parser);
}

// Helper function to parse a block.
AstNode *parse_block(Parser *parser) {
  advance_with_expect(parser, TOKEN_LBRACE);
  AstNode *block_statement =
      ast_new(AST_BLOCK_STATEMENT, parser->current_token);
  block_statement->as.block_statement.statements.data = NULL;
  block_statement->as.block_statement.statements.length = 0;
  block_statement->as.block_statement.statements.capacity = 0;

  while (!check(parser, TOKEN_RBRACE) && !check(parser, TOKEN_EOF)) {
    AstNode *statement = parse_statement(parser);
    if (statement) {
      vec_push(AstNode *, &block_statement->as.block_statement.statements,
               statement);
    }
  }

  advance_with_expect(parser, TOKEN_RBRACE);
  return block_statement;
}

// Helper function to parse function declaration.
AstNode *parse_function_declaration(Parser *parser) {
  Token return_type = advance_parser(parser);
  Token fn_name = advance_with_expect(parser, TOKEN_IDENTIFIER);

  advance_with_expect(parser, TOKEN_LPAREN);
  // TODO: Add ability to parse parameters.
  advance_with_expect(parser, TOKEN_RPAREN);

  // Parsing the funciton block.
  AstNode *block = parse_block(parser);

  // Creating a function node.
  AstNode *fn_node = ast_new(AST_FUNCTION_DECLARATION, return_type);
  fn_node->as.function_declaration.return_type = return_type;
  fn_node->as.function_declaration.fn_name = fn_name;
  fn_node->as.function_declaration.parameters.data = NULL;
  fn_node->as.function_declaration.parameters.length = 0;
  fn_node->as.function_declaration.parameters.capacity = 0;
  fn_node->as.function_declaration.block = block;

  return fn_node;
}

// Helper function to parse declarations.
AstNode *parse_declarations(Parser *parser) {
  if (is_primitive_type(parser->current_token.kind)) {
    return parse_function_declaration(parser);
  }

  // TODO: Remove this
  exit(1);
}

// Parse translation unit.
AstNode *parse_translation_unit(Parser *parser) {
  AstNode *unit = ast_new(AST_TRANSLATION_UNIT, parser->current_token);
  unit->as.translation_unit.declarations.data = NULL;
  unit->as.translation_unit.declarations.length = 0;
  unit->as.translation_unit.declarations.capacity = 0;

  while (!check(parser, TOKEN_EOF)) {
    vec_push(AstNode *, &unit->as.translation_unit.declarations,
             parse_declarations(parser));
  }

  return unit;
}
