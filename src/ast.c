#include "include/ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Function to generate Abstract Syntax Tree (AST).
AstNode *ast_new(AstNodeKind kind, Token token) {
  // Allocate memory for the AST.
  AstNode *node = (AstNode *)(malloc(sizeof(AstNode)));

  // Initialise newly setted memory to 0.
  memset(node, 0, sizeof(*node));

  // Return the node.
  node->kind = kind;
  node->token = token;
  return node;
}

// Helper function to print text with indent.
void print_with_indent(const char *data, int indent) {
  printf("%*c%s", indent, ' ', data);
}

// Function to print AST to the console.
void ast_print(const AstNode *node, int indent) {
  switch (node->kind) {
  case AST_INT_LITERAL_EXPRESSION:
    print_with_indent("AST_INT_LITERAL_EXPRESSION\n", indent);
    break;

  case AST_IDENTIFIER: {
    char *s = (char *)malloc(node->as.identifier.token.length + 1);
    sprintf(s, "%s(%.*s)\n", "AST_IDENTIFIER",
            (int)node->as.identifier.token.length,
            node->as.identifier.token.start_ptr);

    print_with_indent(s, indent);
    free(s);
  } break;

  case AST_PARAMETER: {
    char *s = (char *)malloc(node->as.parameter.parameter_name.length +
                             node->as.parameter.parameter_type.length + 1);

    sprintf(s, "-> %.*s(%.*s)\n", (int)node->as.parameter.parameter_name.length,
            node->as.parameter.parameter_name.start_ptr,
            (int)node->as.parameter.parameter_type.length,
            node->as.parameter.parameter_type.start_ptr);

    print_with_indent(s, indent);
    free(s);
  } break;

  case AST_BLOCK_STATEMENT: {
    print_with_indent("AST_BLOCK_STATEMENT {\n", indent);
    for (int i = 0; i < node->as.block_statement.statements.length; i++) {
      ast_print(node->as.block_statement.statements.data[i], indent + 2);
    }
    print_with_indent("}\n", indent);
  } break;

  case AST_FUNCTION_DECLARATION: {
    char *s =
        (char *)malloc(node->as.function_declaration.fn_name.length +
                       node->as.function_declaration.return_type.length + 1);
    sprintf(s, "%.*s(%.*s)\n",
            (int)node->as.function_declaration.fn_name.length,
            node->as.function_declaration.fn_name.start_ptr,
            (int)node->as.function_declaration.return_type.length,
            node->as.function_declaration.return_type.start_ptr);

    print_with_indent(s, indent);
    free(s);

    for (int i = 0; i < node->as.function_declaration.parameters.length; i++) {
      ast_print(node->as.function_declaration.parameters.data[i], indent + 2);
    }
    ast_print(node->as.function_declaration.block, indent);
  }; break;

  case AST_RETURN_STATEMENT: {
    print_with_indent("AST_RETURN_STATEMENT: ", indent);
    ast_print(node->as.return_statement.value, indent + 2);
  } break;

  case AST_TRANSLATION_UNIT:
    print_with_indent("AST_TRANSLATION_UNTI", indent);
    for (int i = 0; i < node->as.translation_unit.declarations.length; i++) {
      ast_print(node->as.translation_unit.declarations.data[i], indent + 2);
    }
    break;
  }
};