#include "include/ast.h"
#include "include/lexer.h"
#include "llvm-c/Analysis.h"
#include "llvm-c/Core.h"
#include "llvm-c/Types.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Symbol table for function lookups
typedef struct {
  char *name;
  LLVMValueRef function;
} FunctionEntry;

typedef struct {
  FunctionEntry *functions;
  size_t count;
  size_t capacity;
} SymbolTable;

static SymbolTable symbol_table = {0};

// Helper function to add function to symbol table
void add_function_to_symbol_table(const char *name, LLVMValueRef function) {
  if (symbol_table.count >= symbol_table.capacity) {
    symbol_table.capacity =
        symbol_table.capacity == 0 ? 8 : symbol_table.capacity * 2;
    symbol_table.functions = realloc(
        symbol_table.functions, symbol_table.capacity * sizeof(FunctionEntry));
  }

  symbol_table.functions[symbol_table.count].name = strdup(name);
  symbol_table.functions[symbol_table.count].function = function;
  symbol_table.count++;
}

// Helper function to find function in symbol table
LLVMValueRef find_function_in_symbol_table(const char *name) {
  for (size_t i = 0; i < symbol_table.count; i++) {
    if (strcmp(symbol_table.functions[i].name, name) == 0) {
      return symbol_table.functions[i].function;
    }
  }
  return NULL;
}

// Helper function to substring a string.
char *substring(const char *base, size_t length) {
  char *result = malloc(length + 1);
  if (!result) {
    fprintf(stderr, "Memory allocation failed\n");
    exit(1);
  }
  strncpy(result, base, length);
  result[length] = '\0';
  return result;
}

// Helper function to convert primitive type to LLVM type.
LLVMTypeRef
get_llvm_equivalent_for_primitive_type(Token primitive_type_token,
                                       LLVMContextRef llvm_context) {
  switch (primitive_type_token.kind) {
  case TOKEN_VOID:
    return LLVMVoidTypeInContext(llvm_context);
  case TOKEN_INT:
    return LLVMInt8TypeInContext(llvm_context);
    break;

  case TOKEN_STRING: {
    LLVMTypeRef i8_ptr =
        LLVMPointerType(LLVMInt8TypeInContext(llvm_context), 0);
    LLVMTypeRef len_type = LLVMInt8TypeInContext(llvm_context);
    LLVMTypeRef members[] = {i8_ptr, len_type};
    return LLVMStructTypeInContext(llvm_context, members, 2, false);
  } break;

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
    char *value_str = substring(node->as.literal.token.start_ptr,
                                node->as.literal.token.length);
    int value = atoi(value_str);
    free(value_str);

    LLVMTypeRef i8 = LLVMInt8TypeInContext(llvm_context);
    return LLVMConstInt(i8, value, 0);
  } break;

  case AST_STRING_LITERAL_EXPRESSION: {
    // Strip quotes
    char *raw = substring(node->as.string_literal.token.start_ptr + 1,
                          node->as.string_literal.token.length - 2);

    size_t len = strlen(raw);

    // Create a global constant string
    LLVMValueRef str_ptr = LLVMBuildGlobalStringPtr(builder, raw, "str");
    free(raw);

    LLVMTypeRef i8_ptr =
        LLVMPointerType(LLVMInt8TypeInContext(llvm_context), 0);
    LLVMTypeRef len_type = LLVMInt8TypeInContext(llvm_context);
    LLVMTypeRef members[] = {i8_ptr, len_type};
    LLVMTypeRef string_type =
        LLVMStructTypeInContext(llvm_context, members, 2, false);

    LLVMValueRef tmp = LLVMGetUndef(string_type);
    tmp = LLVMBuildInsertValue(builder, tmp, str_ptr, 0, "str_data");
    tmp = LLVMBuildInsertValue(builder, tmp, LLVMConstInt(len_type, len, 0), 1,
                               "str_len");

    return tmp;
  } break;

  case AST_CALL_EXPRESSION: {
    char *fn_name = substring(node->as.call_expression.callee->token.start_ptr,
                              node->as.call_expression.callee->token.length);

    LLVMValueRef function = find_function_in_symbol_table(fn_name);
    if (!function) {
      fprintf(stderr, "Error: Function '%s' not found\n", fn_name);
      free(fn_name);
      exit(1);
    }

    LLVMValueRef *args = NULL;
    if (node->as.call_expression.arguments.length > 0) {
      args = malloc(node->as.call_expression.arguments.length *
                    sizeof(LLVMValueRef));

      for (size_t i = 0; i < node->as.call_expression.arguments.length; i++) {
        LLVMValueRef value =
            convert_statement(node->as.call_expression.arguments.data[i],
                              llvm_module, llvm_context, builder);

        if (LLVMGetTypeKind(LLVMTypeOf(value)) == LLVMStructTypeKind) {
          value = LLVMBuildExtractValue(builder, value, 0, "str_data");
        }
        args[i] = value;
      }
    }

    LLVMValueRef call_result =
        LLVMBuildCall2(builder, LLVMGlobalGetValueType(function), function,
                       args, node->as.call_expression.arguments.length, "");

    if (args)
      free(args);
    free(fn_name);

    LLVMTypeRef fn_type = LLVMGlobalGetValueType(function);
    LLVMTypeRef return_type = LLVMGetReturnType(fn_type);
    if (LLVMGetTypeKind(return_type) == LLVMVoidTypeKind)
      return NULL; // Don't propagate result if it's void

    return call_result;
  } break;

  case AST_RETURN_STATEMENT: {
    if (node->as.return_statement.value) {
      LLVMValueRef return_value = convert_statement(
          node->as.return_statement.value, llvm_module, llvm_context, builder);
      LLVMBuildRet(builder, return_value);
    } else {
      // For void functions
      LLVMBuildRetVoid(builder);
    }
    return NULL;
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

    // Create parameter types array
    LLVMTypeRef *param_types = NULL;
    unsigned param_count = node->as.function_declaration.parameters.length;

    if (param_count > 0) {
      param_types = malloc(param_count * sizeof(LLVMTypeRef));
      for (unsigned i = 0; i < param_count; i++) {
        AstNode *param = node->as.function_declaration.parameters.data[i];
        param_types[i] = get_llvm_equivalent_for_primitive_type(
            param->as.parameter.parameter_type, llvm_context);
      }
    }

    // Create function type with parameters
    LLVMTypeRef function_type =
        LLVMFunctionType(llvm_return_type, param_types, param_count, 0);

    // Defining a function.
    char *fn_name =
        (char *)malloc(node->as.function_declaration.fn_name.length + 1);
    sprintf(fn_name, "%.*s", (int)node->as.function_declaration.fn_name.length,
            node->as.function_declaration.fn_name.start_ptr);
    LLVMValueRef fn = LLVMAddFunction(llvm_module, fn_name, function_type);

    // Add function to symbol table
    add_function_to_symbol_table(fn_name, fn);

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

    // Free allocated memory
    if (param_types) {
      free(param_types);
    }
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

  // Clear symbol table
  symbol_table.count = 0;

  // declare all functions
  for (int i = 0;
       i < (int)translation_unit->as.translation_unit.declarations.length;
       i++) {
    AstNode *node = translation_unit->as.translation_unit.declarations.data[i];

    if (node->kind == AST_FOREIGN_DECLARATION) {
      // Building the function return type.
      Token return_type = node->as.foreign_declaration.return_type;
      LLVMTypeRef llvm_return_type =
          get_llvm_equivalent_for_primitive_type(return_type, llvm_context);

      // Setting up parameter types.
      LLVMTypeRef *param_types = NULL;
      unsigned param_count = node->as.foreign_declaration.parameters.length;

      if (param_count > 0) {
        param_types = malloc(param_count * sizeof(LLVMTypeRef));
        for (unsigned i = 0; i < param_count; i++) {
          AstNode *param = node->as.foreign_declaration.parameters.data[i];

          if (param->as.parameter.parameter_type.kind == TOKEN_STRING) {
            param_types[i] =
                LLVMPointerType(LLVMInt8TypeInContext(llvm_context), 0);
          } else {
            param_types[i] = get_llvm_equivalent_for_primitive_type(
                param->as.parameter.parameter_type, llvm_context);
          }
        }
      }

      // Building the function.
      LLVMTypeRef fn_type =
          LLVMFunctionType(llvm_return_type, param_types, param_count, 0);

      char *fn_name =
          substring(node->as.foreign_declaration.symbol_name.start_ptr + 1,
                    node->as.foreign_declaration.symbol_name.length - 2);

      LLVMValueRef fn = LLVMAddFunction(llvm_module, fn_name, fn_type);
      add_function_to_symbol_table(fn_name, fn);

      if (param_types)
        free(param_types);
      free(fn_name);
      continue;
    }

    else if (node->kind == AST_FUNCTION_DECLARATION) {
      Token return_type_token = node->as.function_declaration.return_type;
      LLVMTypeRef llvm_return_type = get_llvm_equivalent_for_primitive_type(
          return_type_token, llvm_context);

      // Create parameter types array
      LLVMTypeRef *param_types = NULL;
      unsigned param_count = node->as.function_declaration.parameters.length;

      if (param_count > 0) {
        param_types = malloc(param_count * sizeof(LLVMTypeRef));
        for (unsigned j = 0; j < param_count; j++) {
          AstNode *param = node->as.function_declaration.parameters.data[j];
          param_types[j] = get_llvm_equivalent_for_primitive_type(
              param->as.parameter.parameter_type, llvm_context);
        }
      }

      // Create function type with parameters
      LLVMTypeRef function_type =
          LLVMFunctionType(llvm_return_type, param_types, param_count, 0);

      // Defining a function.
      char *fn_name = substring(node->as.function_declaration.fn_name.start_ptr,
                                node->as.function_declaration.fn_name.length);
      LLVMValueRef fn = LLVMAddFunction(llvm_module, fn_name, function_type);

      // Add function to symbol table
      add_function_to_symbol_table(fn_name, fn);

      // Free allocated memory
      if (param_types) {
        free(param_types);
      }
      free(fn_name);
    }
  }

  // implement function bodies
  for (int i = 0;
       i < (int)translation_unit->as.translation_unit.declarations.length;
       i++) {
    AstNode *node = translation_unit->as.translation_unit.declarations.data[i];
    if (node->kind == AST_FUNCTION_DECLARATION) {
      char *fn_name = substring(node->as.function_declaration.fn_name.start_ptr,
                                node->as.function_declaration.fn_name.length);
      LLVMValueRef fn = find_function_in_symbol_table(fn_name);

      // Defining the function block.
      LLVMBasicBlockRef fn_main =
          LLVMAppendBasicBlockInContext(llvm_context, fn, "entry");

      // Positioning the builder in the function block.
      LLVMPositionBuilderAtEnd(builder, fn_main);

      // Converting the statements to IR.
      bool has_return = false;
      AstNode *block_node = node->as.function_declaration.block;
      for (int j = 0; j < (int)block_node->as.block_statement.statements.length;
           j++) {
        AstNode *stmt = block_node->as.block_statement.statements.data[j];

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

      free(fn_name);
    }
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

  // Clean up symbol table
  for (size_t i = 0; i < symbol_table.count; i++) {
    free(symbol_table.functions[i].name);
  }
  if (symbol_table.functions) {
    free(symbol_table.functions);
    symbol_table.functions = NULL;
    symbol_table.count = 0;
    symbol_table.capacity = 0;
  }

  return ir;
}