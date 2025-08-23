#ifndef FERRO_LANG_CODEGEN
#define FERRO_LANG_CODEGEN

#include "ast.h"

// Function to generate LLVM IR.
const char *codegen(AstNode *translation_unit);

#endif