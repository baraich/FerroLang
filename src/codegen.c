#include "include/ast.h"
#include "include/lexer.h"
#include "llvm-c/Analysis.h"
#include "llvm-c/Core.h"
#include "llvm-c/Types.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Helper function to substring a string.
const char *substring(const char *base, size_t length) {
  char *substring = (char *)malloc(length + 1);

  // Copying a part of string to a @substring
  strncpy(substring, base, length);
  substring[length] = '\0';

  // Returning the substring;
  return substring;
}

// Helper function to convert primitive type to LLVM type.
LLVMTypeRef
get_llvm_equivalent_for_primitive_type(Token primitive_type_token,
                                       LLVMContextRef llvm_context) {
  switch (primitive_type_token.kind) {
  case TOKEN_INT:
    return LLVMInt8TypeInContext(llvm_context);
    break;

  default:
    fprintf(stderr, "Error: %s is not a primitive type.",
            token_kind_to_string(primitive_type_token.kind));
    exit(1);
  }
}

LLVMValueRef convert_statement(AstNode *node, LLVMModuleRef llvm_module,
                               LLVMContextRef llvm_context,
                               LLVMBuilderRef builder) {
  switch (node->kind) {
  case AST_INT_LITERAL_EXPRESSION: {
    int value = atoi(substring(node->as.literal.token.start_ptr,
                               node->as.literal.token.length));

    // Building the llvm value.
    LLVMTypeRef i8 = LLVMInt8TypeInContext(llvm_context);
    return LLVMConstInt(i8, value,
                        0); // Fixed: third parameter should be 0 (signed)
  } break;
  case AST_RETURN_STATEMENT: {
    // Building a return statement in IR.
    AstNode *return_node = node->as.return_statement.value;
    LLVMValueRef return_value =
        convert_statement(return_node, llvm_module, llvm_context, builder);
    LLVMBuildRet(builder, return_value);
    return return_value; // Return the value for consistency
  } break;
  default:
    printf("Should not have reached this part - statement\n");
    exit(1);
  }
}

// Helper function to convert a node to IR.
void convert_declaration(AstNode *node, LLVMModuleRef llvm_module,
                         LLVMContextRef llvm_context, LLVMBuilderRef builder) {
  switch (node->kind) {
  case AST_FUNCTION_DECLARATION: {
    Token return_type_token = node->as.function_declaration.return_type;
    LLVMTypeRef llvm_return_type =
        get_llvm_equivalent_for_primitive_type(return_type_token, llvm_context);

    // Create function type (no parameters for now)
    LLVMTypeRef function_type = LLVMFunctionType(llvm_return_type, NULL, 0, 0);

    // Defining a function.
    char *fn_name =
        (char *)malloc(node->as.function_declaration.fn_name.length + 1);
    sprintf(fn_name, "%.*s", (int)node->as.function_declaration.fn_name.length,
            node->as.function_declaration.fn_name.start_ptr);
    LLVMValueRef fn = LLVMAddFunction(llvm_module, fn_name, function_type);

    // Defining the function block.
    LLVMBasicBlockRef fn_main =
        LLVMAppendBasicBlockInContext(llvm_context, fn, "entry");

    // Positioning the builder in the function block.
    LLVMPositionBuilderAtEnd(builder, fn_main);

    // Converting the statements to IR.
    bool has_return = false;
    AstNode *block_node = node->as.function_declaration.block;
    for (int i = 0; i < (int)block_node->as.block_statement.statements.length;
         i++) {
      AstNode *stmt = block_node->as.block_statement.statements.data[i];

      // Fixed: Check the statement's kind, not the block's kind
      if (stmt->kind == AST_RETURN_STATEMENT) {
        has_return = true;
        convert_statement(stmt, llvm_module, llvm_context, builder);
        break;
      } else {
        convert_statement(stmt, llvm_module, llvm_context, builder);
      }
    }

    // If no return statement was found, add a default return
    if (!has_return) {
      LLVMTypeRef type = LLVMInt8TypeInContext(llvm_context);
      LLVMValueRef default_return = LLVMConstInt(type, 1, 0);
      LLVMBuildRet(builder, default_return);
    }

    // Free the allocated function name
    free(fn_name);
  } break;
  default:
    fprintf(stderr, "Should not have reached here.\n");
    exit(1);
  }
}

// Function to generate LLVM IR.
const char *codegen(AstNode *translation_unit) {
  if (translation_unit->kind != AST_TRANSLATION_UNIT) {
    printf("Provided node is not a translation unit.\n");
    exit(1);
  }

  // Creating LLVM context and module.
  LLVMContextRef llvm_context = LLVMContextCreate();
  LLVMModuleRef llvm_module =
      LLVMModuleCreateWithNameInContext("main_module", llvm_context);

  // Creating an IR builder.
  LLVMBuilderRef builder = LLVMCreateBuilderInContext(llvm_context);

  // Building the program.
  for (int i = 0;
       i < (int)translation_unit->as.translation_unit.declarations.length;
       i++) {
    AstNode *node = translation_unit->as.translation_unit.declarations.data[i];
    convert_declaration(node, llvm_module, llvm_context, builder);
  }

  // Returning the IR back.
  char *err = NULL;
  if (LLVMVerifyModule(llvm_module, LLVMReturnStatusAction, &err)) {
    fprintf(stderr, "Failed to verify the module: %s\n", err);
    LLVMDisposeMessage(err);
    LLVMDisposeBuilder(builder);
    LLVMDisposeModule(llvm_module);
    LLVMContextDispose(llvm_context);
    exit(1);
  }
  if (err)
    LLVMDisposeMessage(err);

  char *ir = LLVMPrintModuleToString(llvm_module);

  // Clean up resources
  LLVMDisposeBuilder(builder);
  LLVMDisposeModule(llvm_module);
  LLVMContextDispose(llvm_context);

  return ir;
}