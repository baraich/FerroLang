#ifndef FERRO_LANG_AST
#define FERRO_LANG_AST

#include "helpers.h"
#include "lexer.h"

// Possible node categories.
typedef enum {
  // Declarations.
  AST_TRANSLATION_UNIT,
  AST_FUNCTION_DECLARATION,

  // Node
  AST_PARAMETER,

  // Statements
  AST_BLOCK_STATEMENT,
  AST_RETURN_STATEMENT,

  // Expressions
  AST_INT_LITERAL_EXPRESSION,
  AST_CALL_EXPRESSION,
  AST_IDENTIFIER_EXPRESSION
} AstNodeKind;

// Forward declaration for AstNode.
struct AstNode;
typedef struct AstNode AstNode;

// Represents integers,
// FUTURE: floats, booleans and strings.
typedef struct {
  Token token;
} AstLiteral;

// Represents an identifer.
typedef struct {
  Token token;
} AstIdentifer;

// Represents a parameter.
typedef struct {
  Token parameter_type;
  Token parameter_name;
} AstParameter;

// Represents a function.
typedef struct {
  Token return_type;
  Token fn_name;
  Vector(AstNode *) parameters;
  AstNode *block;
} AstFunctionDeclaration;

// Represents a block.
typedef struct {
  Vector(AstNode *) statements;
} AstBlockStatement;

// Represents a return statement.
typedef struct {
  AstNode *value;
} AstReturnStatement;

// Represents whole program.
typedef struct {
  Vector(AstNode *) declarations;
} AstTranslationUnit;

// Represents a function call.
typedef struct {
  AstNode *callee;
  Vector(AstNode *) arguments;
} AstCallExpression;

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
  } as;
};

// Function to generate Abstract Syntax Tree (AST).
AstNode *ast_new(AstNodeKind kind, Token token);

// Function to print AST to the console.
void ast_print(const AstNode *node, int indent);
void ast_free(AstNode *node);

#endif