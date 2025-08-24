#include "include/ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Function to generate Abstract Syntax Tree (AST).
AstNode *ast_new(AstNodeKind kind, Token token) {
  AstNode *node = malloc(sizeof(AstNode));
  if (!node) {
    fprintf(stderr, "Memory allocation failed\n");
    exit(1);
  }

  memset(node, 0, sizeof(*node));
  node->kind = kind;
  node->token = token;
  return node;
}

// Helper function to print text with indent.
void print_with_indent(const char *data, int indent) {
  if (indent == 0) {
    printf("%s", data);
  } else {
    printf("%*c%s", indent, ' ', data);
  }
}

// Function to print AST to the console.
void ast_print(const AstNode *node, int indent) {
  switch (node->kind) {
  case AST_FOREIGN_DECLARATION: {

    char *s = (char *)malloc(node->as.foreign_declaration.fn_name.length + 1);
    sprintf(s, "%.*s", (int)node->as.foreign_declaration.fn_name.length,
            node->as.foreign_declaration.fn_name.start_ptr);
    print_with_indent("AST_FOREIGN_DECLARATION(", indent);
    print_with_indent(s, 0);
    print_with_indent(")\n", 0);
  } break;
  case AST_STRING_LITERAL_EXPRESSION: {
    size_t buffer_size = (int)node->as.string_literal.token.length +
                         50; // Extra space for format
    char *s = (char *)malloc(buffer_size);
    if (!s) {
      fprintf(stderr, "Memory allocation failed\n");
      exit(1);
    }

    // Copy the literal value from the token
    snprintf(s, buffer_size, "%.*s", (int)node->as.string_literal.token.length,
             node->as.string_literal.token.start_ptr);

    // Print the literal value of the token
    print_with_indent("AST_STRING_LITERAL_EXPRESSION(", indent);
    print_with_indent(s, 0); // Prints the actual value
    print_with_indent(")\n", 0);

    // Free the allocated string after printing
    free(s);
  } break;
    ;
  case AST_INT_LITERAL_EXPRESSION: {
    size_t buffer_size =
        (int)node->as.literal.token.length + 50; // Extra space for format
    char *s = (char *)malloc(buffer_size);
    if (!s) {
      fprintf(stderr, "Memory allocation failed\n");
      exit(1);
    }

    // Copy the literal value from the token
    snprintf(s, buffer_size, "%.*s", (int)node->as.literal.token.length,
             node->as.literal.token.start_ptr);

    // Print the literal value of the token
    print_with_indent("AST_INT_LITERAL_EXPRESSION(", indent);
    print_with_indent(s, 0); // Prints the actual value
    print_with_indent(")\n", 0);

    // Free the allocated string after printing
    free(s);
  } break;

  case AST_IDENTIFIER_EXPRESSION: {
    size_t buffer_size =
        node->as.identifier.token.length + 50; // Extra space for format string
    char *s = (char *)malloc(buffer_size);
    if (!s) {
      fprintf(stderr, "Memory allocation failed\n");
      exit(1);
    }
    snprintf(s, buffer_size, "%s(%.*s)\n", "AST_IDENTIFIER_EXPRESSION",
             (int)node->as.identifier.token.length,
             node->as.identifier.token.start_ptr);

    print_with_indent(s, indent);
    free(s);
  } break;

  case AST_PARAMETER: {
    size_t name_len = node->as.parameter.parameter_name.length;
    size_t type_len = node->as.parameter.parameter_type.length;
    size_t buffer_size = name_len + type_len + 20; // Extra space for format

    char *s = malloc(buffer_size);
    if (!s) {
      fprintf(stderr, "Memory allocation failed\n");
      exit(1);
    }

    snprintf(s, buffer_size, "-> %.*s(%.*s)\n", (int)type_len,
             node->as.parameter.parameter_type.start_ptr, (int)name_len,
             node->as.parameter.parameter_name.start_ptr);

    print_with_indent(s, indent);
    free(s);
  } break;

  case AST_BLOCK_STATEMENT: {
    print_with_indent("AST_BLOCK_STATEMENT {\n", indent);
    for (int i = 0; i < (int)node->as.block_statement.statements.length; i++) {
      ast_print(node->as.block_statement.statements.data[i], indent + 2);
    }
    print_with_indent("}\n", indent);
  } break;

  case AST_FUNCTION_DECLARATION: {
    size_t buffer_size = node->as.function_declaration.fn_name.length +
                         node->as.function_declaration.return_type.length + 50;
    char *s = (char *)malloc(buffer_size);
    if (!s) {
      fprintf(stderr, "Memory allocation failed\n");
      exit(1);
    }

    snprintf(s, buffer_size, "AST_FUNCTION_DECLARATION %.*s(%.*s)\n",
             (int)node->as.function_declaration.fn_name.length,
             node->as.function_declaration.fn_name.start_ptr,
             (int)node->as.function_declaration.return_type.length,
             node->as.function_declaration.return_type.start_ptr);

    print_with_indent(s, indent);
    free(s);

    // Print parameters
    if (node->as.function_declaration.parameters.length > 0) {
      print_with_indent("Parameters:\n", indent + 2);
      for (int i = 0; i < (int)node->as.function_declaration.parameters.length;
           i++) {
        ast_print(node->as.function_declaration.parameters.data[i], indent + 4);
      }
    }

    // Print function body
    ast_print(node->as.function_declaration.block, indent + 2);
  }; break;

  case AST_RETURN_STATEMENT: {
    print_with_indent("AST_RETURN_STATEMENT: \n", indent);
    ast_print(node->as.return_statement.value, indent + 2);
  } break;

  case AST_CALL_EXPRESSION: {
    print_with_indent("AST_CALL_EXPRESSION\n", indent);

    size_t buffer_size = node->as.call_expression.callee->token.length + 20;
    char *s = (char *)malloc(buffer_size);
    if (!s) {
      fprintf(stderr, "Memory allocation failed\n");
      exit(1);
    }

    snprintf(s, buffer_size, "Callee: %.*s\n",
             (int)node->as.call_expression.callee->token.length,
             node->as.call_expression.callee->token.start_ptr);

    print_with_indent(s, indent + 2);
    free(s);

    // Print arguments if any
    if (node->as.call_expression.arguments.length > 0) {
      print_with_indent("Arguments:\n", indent + 2);
      for (int i = 0; i < (int)node->as.call_expression.arguments.length; i++) {
        ast_print(node->as.call_expression.arguments.data[i], indent + 4);
      }
    }
  } break;

  case AST_TRANSLATION_UNIT:
    print_with_indent("AST_TRANSLATION_UNIT\n", indent);
    for (int i = 0; i < (int)node->as.translation_unit.declarations.length;
         i++) {
      ast_print(node->as.translation_unit.declarations.data[i], indent + 2);
    }
    break;
  }
};

void ast_free(AstNode *node) {
  if (!node)
    return;

  switch (node->kind) {
  case AST_TRANSLATION_UNIT:
    for (size_t i = 0; i < node->as.translation_unit.declarations.length; i++) {
      ast_free(node->as.translation_unit.declarations.data[i]);
    }
    vec_free(AstNode *, &node->as.translation_unit.declarations);
    break;

  case AST_FUNCTION_DECLARATION:
    for (size_t i = 0; i < node->as.function_declaration.parameters.length;
         i++) {
      ast_free(node->as.function_declaration.parameters.data[i]);
    }
    vec_free(AstNode *, &node->as.function_declaration.parameters);
    ast_free(node->as.function_declaration.block);
    break;

  case AST_BLOCK_STATEMENT:
    for (size_t i = 0; i < node->as.block_statement.statements.length; i++) {
      ast_free(node->as.block_statement.statements.data[i]);
    }
    vec_free(AstNode *, &node->as.block_statement.statements);
    break;

  case AST_CALL_EXPRESSION: {
    ast_free(node->as.call_expression.callee);
    for (size_t i = 0; i < node->as.call_expression.arguments.length; i++) {
      ast_free(node->as.call_expression.arguments.data[i]);
    }
    vec_free(AstNode *, &node->as.call_expression.arguments);
  } break;

  case AST_RETURN_STATEMENT:
    ast_free(node->as.return_statement.value);
    break;

  // Other cases don't have child nodes to free
  default:
    break;
  }

  free(node);
}