#ifndef FERRO_LANG_AST
#define FERRO_LANG_AST

#include "helpers.h"
#include "lexer.h"
#include "stdbool.h"

// Possible node categories.
typedef enum {
  // Declarations.
  AST_TRANSLATION_UNIT,
  AST_FUNCTION_DECLARATION,
  AST_FOREIGN_DECLARATION,

  // Node
  AST_PARAMETER,

  // Statements
  AST_BLOCK_STATEMENT,
  AST_RETURN_STATEMENT,

  // Expressions
  AST_INT_LITERAL_EXPRESSION,
  AST_STRING_LITERAL_EXPRESSION,
  AST_CALL_EXPRESSION,
  AST_IDENTIFIER_EXPRESSION
} AstNodeKind;

// Forward declaration for AstNode.
struct AstNode;
typedef struct AstNode AstNode;

// Named vector type for AST nodes to avoid anonymous-struct incompatibilities
typedef Vector(AstNode *) AstNodeVector;

// Represents integers,
// FUTURE: floats, boolean.
typedef struct {
  Token token;
} AstLiteral;

// Represents a string.
typedef struct {
  Token token;
} AstStringLiteral;

// Represents an identifer.
typedef struct {
  Token token;
} AstIdentifer;

// Represents a parameter.
typedef struct {
  Token parameter_type;
  Token parameter_name;
  bool is_tail_parameter;
} AstParameter;

// Represents a function.
typedef struct {
  Token return_type;
  Token fn_name;
  AstNode *block;
  bool has_tail_arg;
  AstNodeVector parameters;
} AstFunctionDeclaration;

// Represents a block.
typedef struct {
  AstNodeVector statements;
} AstBlockStatement;

// Represents a return statement.
typedef struct {
  AstNode *value;
} AstReturnStatement;

// Represents whole program.
typedef struct {
  AstNodeVector declarations;
} AstTranslationUnit;

// Represents a function call.
typedef struct {
  AstNode *callee;
  AstNodeVector arguments;
} AstCallExpression;

// Represents a foreign function.
typedef struct {
  Token return_type;
  Token fn_name;
  Token source_path;
  Token symbol_name;
  AstNodeVector parameters;
} AstForeignDeclaration;

struct AstNode {
  Token token;
  AstNodeKind kind;
  union {
    AstTranslationUnit translation_unit;
    AstFunctionDeclaration function_declaration;
    AstBlockStatement block_statement;
    AstParameter parameter;
    AstLiteral literal;
    AstReturnStatement return_statement;
    AstIdentifer identifier;
    AstCallExpression call_expression;
    AstForeignDeclaration foreign_declaration;
    AstStringLiteral string_literal;
  } as;
};

// Function to generate Abstract Syntax Tree (AST).
AstNode *ast_new(AstNodeKind kind, Token token);

// Function to print AST to the console.
void ast_print(const AstNode *node, int indent);
void ast_free(AstNode *node);

#endif