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
  case TOKEN_STRING:
    return true;
  case TOKEN_VOID:
    return true;
  default:
    return false;
  }
}

// Helper function to advance the parser with expect.
Token advance_with_expect(Parser *parser, TokenKind expected_token_kind) {
  if (check(parser, expected_token_kind)) {
    return advance_parser(parser);
  }

  fprintf(stderr, "Parse error: Expected %s but got %s at line %zu\n",
          token_kind_to_string(expected_token_kind),
          token_kind_to_string(parser->current_token.kind),
          parser->current_token.line);
  exit(1);
}

// Helper function to parse a parameter.
AstNode *parse_parameter(Parser *parser) {
  // Expect a primitive type first
  if (!is_primitive_type(parser->current_token.kind)) {
    fprintf(stderr,
            "Parse error: Expected primitive type for parameter at line %zu\n",
            parser->current_token.line);
    exit(1);
  }

  Token type_token = advance_parser(parser);
  bool is_tail_parameter = false;
  if (check(parser, TOKEN_TAIL)) {
    is_tail_parameter = true;
    advance_with_expect(parser, TOKEN_TAIL);
  }
  Token name_token = advance_with_expect(parser, TOKEN_IDENTIFIER);

  AstNode *param_node = ast_new(AST_PARAMETER, type_token);
  param_node->as.parameter.parameter_type = type_token;
  param_node->as.parameter.parameter_name = name_token;
  param_node->as.parameter.is_tail_parameter = is_tail_parameter;

  return param_node;
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
  case TOKEN_STRING_LITERAL: {
    Token t = advance_parser(parser);
    AstNode *node = ast_new(AST_STRING_LITERAL_EXPRESSION, t);
    node->as.string_literal.token = t;
    return node;
  }
  case TOKEN_IDENTIFIER: {
    Token t = advance_parser(parser);

    // Check if this is a function call
    if (check(parser, TOKEN_LPAREN)) {
      // This is a function call
      advance_parser(parser); // consume '('

      AstNode *call_node = ast_new(AST_CALL_EXPRESSION, t);

      // Create callee node (identifier)
      AstNode *callee = ast_new(AST_IDENTIFIER_EXPRESSION, t);
      callee->as.identifier.token = t;
      call_node->as.call_expression.callee = callee;

      // Initialize arguments vector
      vec_init(AstNode *, &call_node->as.call_expression.arguments);

      // Parse arguments if any
      if (!check(parser, TOKEN_RPAREN)) {
        do {
          AstNode *arg = parse_expression(parser);
          vec_push(AstNode *, &call_node->as.call_expression.arguments, arg);

          if (check(parser, TOKEN_COMMA)) {
            advance_parser(parser); // consume ','
          } else {
            break;
          }
        } while (true);
      }

      advance_with_expect(parser, TOKEN_RPAREN);
      return call_node;
    } else {
      // This is just an identifier
      AstNode *node = ast_new(AST_IDENTIFIER_EXPRESSION, t);
      node->as.identifier.token = t;
      return node;
    }
  }
  default:
    break;
  }

  fprintf(stderr, "Parse error: Unexpected token %s at line %zu\n",
          token_kind_to_string(parser->current_token.kind),
          parser->current_token.line);
  exit(1);
}

// Helper function to parser return statement.
AstNode *parse_return_statement(Parser *parser) {
  // If the next token is a semicolon, it's a bare return
  if (check(parser, TOKEN_SEMICOLON)) {
    AstNode *node = ast_new(AST_RETURN_STATEMENT, parser->current_token);
    node->as.return_statement.value = NULL;
    advance_with_expect(parser, TOKEN_SEMICOLON);
    return node;
  }

  // Otherwise parse the expression
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

  // Parse expression statement
  AstNode *expr = parse_expression(parser);
  advance_with_expect(parser, TOKEN_SEMICOLON);
  return expr;
}

// Helper function to parse a block.
AstNode *parse_block(Parser *parser) {
  advance_with_expect(parser, TOKEN_LBRACE);
  AstNode *block_statement =
      ast_new(AST_BLOCK_STATEMENT, parser->current_token);

  // Use the helper macro instead of manual initialization
  vec_init(AstNode *, &block_statement->as.block_statement.statements);

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

  // Creating a function node.
  AstNode *fn_node = ast_new(AST_FUNCTION_DECLARATION, return_type);
  fn_node->as.function_declaration.return_type = return_type;
  fn_node->as.function_declaration.fn_name = fn_name;

  // Initialize parameters vector
  vec_init(AstNode *, &fn_node->as.function_declaration.parameters);

  // Parse parameters if any
  if (!check(parser, TOKEN_RPAREN)) {
    do {
      AstNode *param = parse_parameter(parser);
      vec_push(AstNode *, &fn_node->as.function_declaration.parameters, param);

      if (check(parser, TOKEN_COMMA)) {
        advance_parser(parser); // consume ','
      } else {
        break;
      }
    } while (true);
  }

  advance_with_expect(parser, TOKEN_RPAREN);

  // Parsing the function block.
  AstNode *block = parse_block(parser);
  fn_node->as.function_declaration.block = block;

  return fn_node;
}

// Helper function to parse foreign function.
AstNode *parse_foreign_declaration(Parser *parser) {
  advance_with_expect(parser, TOKEN_FOREIGN);
  advance_with_expect(parser, TOKEN_LPAREN);

  // Parsing the source file.
  Token source_path = advance_with_expect(parser, TOKEN_STRING_LITERAL);
  advance_with_expect(parser, TOKEN_COMMA);

  // Parse foreign symbol name.
  Token symbol_name = advance_with_expect(parser, TOKEN_STRING_LITERAL);
  advance_with_expect(parser, TOKEN_RPAREN);

  // Return type
  Token return_type = advance_parser(parser);
  // Return type
  if (!is_primitive_type(return_type.kind)) {
    fprintf(stderr,
            "Parse error: Expected primitive type for foreign function return "
            "type at line %zu\n",
            parser->current_token.line);
    exit(1);
  }

  Token fn_name = advance_with_expect(parser, TOKEN_IDENTIFIER);

  advance_with_expect(parser, TOKEN_LPAREN);

  // Building a AST node.
  AstNode *node = ast_new(AST_FOREIGN_DECLARATION, return_type);
  node->as.foreign_declaration.return_type = return_type;
  node->as.foreign_declaration.fn_name = fn_name;
  node->as.foreign_declaration.source_path = source_path;
  node->as.foreign_declaration.symbol_name = symbol_name;

  vec_init(AstNode *, &node->as.foreign_declaration.parameters);

  if (!check(parser, TOKEN_RPAREN)) {
    // Parse parameters
    do {
      AstNode *param = parse_parameter(parser);
      vec_push(AstNode *, &node->as.foreign_declaration.parameters, param);
      if (check(parser, TOKEN_COMMA)) {
        advance_parser(parser);
      } else {
        break;
      }
    } while (true);
  }

  advance_with_expect(parser, TOKEN_RPAREN);
  advance_with_expect(parser, TOKEN_SEMICOLON);

  return node;
}

// Helper function to parse declarations.
AstNode *parse_declarations(Parser *parser) {
  if (check(parser, TOKEN_FOREIGN)) {
    return parse_foreign_declaration(parser);
  }

  if (is_primitive_type(parser->current_token.kind)) {
    return parse_function_declaration(parser);
  }

  return parse_statement(parser);
}

// Parse translation unit.
AstNode *parse_translation_unit(Parser *parser) {
  AstNode *unit = ast_new(AST_TRANSLATION_UNIT, parser->current_token);
  vec_init(AstNode *, &unit->as.translation_unit.declarations);

  while (!check(parser, TOKEN_EOF)) {
    vec_push(AstNode *, &unit->as.translation_unit.declarations,
             parse_declarations(parser));
  }

  return unit;
}