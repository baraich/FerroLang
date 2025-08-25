#include "include/ast.h"
#include "include/helpers.h"
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

char *process_escape_sequences(const char *raw_string, size_t length) {
  char *processed = malloc(length + 1); // Allocate maximum possible size
  if (!processed) {
    fprintf(stderr, "Memory allocation failed\n");
    exit(1);
  }

  size_t write_pos = 0;
  for (size_t read_pos = 0; read_pos < length; read_pos++) {
    if (raw_string[read_pos] == '\\' && read_pos + 1 < length) {
      // Process escape sequence
      switch (raw_string[read_pos + 1]) {
      case 'n':
        processed[write_pos++] = '\n';
        break;
      case 't':
        processed[write_pos++] = '\t';
        break;
      case 'r':
        processed[write_pos++] = '\r';
        break;
      case '\\':
        processed[write_pos++] = '\\';
        break;
      case '"':
        processed[write_pos++] = '"';
        break;
      case '0':
        processed[write_pos++] = '\0';
        break;
      default:
        // If it's not a recognized escape sequence, keep both characters
        processed[write_pos++] = raw_string[read_pos];
        processed[write_pos++] = raw_string[read_pos + 1];
        break;
      }
      read_pos++; // Skip the escaped character
    } else {
      processed[write_pos++] = raw_string[read_pos];
    }
  }

  processed[write_pos] = '\0';
  return processed;
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

    // Process escape sequences
    char *processed = process_escape_sequences(raw, strlen(raw));
    size_t len = strlen(processed);

    // Create a global constant string
    LLVMValueRef str_ptr = LLVMBuildGlobalStringPtr(builder, processed, "str");
    free(raw);
    free(processed);

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
  } break;
  default:
    fprintf(stderr, "Error: Unhandled AST statement kind: %d\n", node->kind);
    exit(1);
  }

  return NULL;
}

// Helper function to create LLVM function signature from parameters
typedef struct {
  LLVMTypeRef function_type;
  LLVMTypeRef *param_types;
  unsigned param_count;
  bool has_tail_arg;
} FunctionSignature;

FunctionSignature create_function_signature(Token return_type,
                                            AstNodeVector parameters,
                                            LLVMContextRef llvm_context,
                                            bool is_foreign) {
  // Build return type
  LLVMTypeRef llvm_return_type =
      get_llvm_equivalent_for_primitive_type(return_type, llvm_context);

  // Setup parameter types
  LLVMTypeRef *param_types = NULL;
  bool has_tail_arg = false;
  unsigned param_count = parameters.length;

  if (param_count > 0) {
    param_types = malloc(param_count * sizeof(LLVMTypeRef));
    for (unsigned i = 0; i < param_count; i++) {
      AstNode *param = parameters.data[i];

      // Check tail parameter constraint (only for non-foreign functions)
      if (param->as.parameter.is_tail_parameter) {
        if (i != param_count - 1) {
          fprintf(
              stderr,
              "Error: Tail parameter must be the last parameter (at position "
              "%d).\n",
              i);
          exit(0);
        }

        has_tail_arg = true;
      }

      // Special handling for foreign functions with string parameters
      if (is_foreign &&
          param->as.parameter.parameter_type.kind == TOKEN_STRING) {
        param_types[i] =
            LLVMPointerType(LLVMInt8TypeInContext(llvm_context), 0);
      } else {
        param_types[i] = get_llvm_equivalent_for_primitive_type(
            param->as.parameter.parameter_type, llvm_context);
      }
    }
  }

  // Create function type
  LLVMTypeRef function_type = LLVMFunctionType(llvm_return_type, param_types,
                                               param_count, has_tail_arg);

  FunctionSignature signature = {.function_type = function_type,
                                 .param_types = param_types,
                                 .param_count = param_count,
                                 .has_tail_arg = has_tail_arg};

  return signature;
}

// Helper function to convert a node to IR.
void convert_declaration(AstNode *node, LLVMModuleRef llvm_module,
                         LLVMContextRef llvm_context, LLVMBuilderRef builder) {
  switch (node->kind) {
  case AST_FOREIGN_DECLARATION: {
    FunctionSignature signature = create_function_signature(
        node->as.foreign_declaration.return_type,
        node->as.foreign_declaration.parameters, llvm_context, true);

    char *source_name =
        substring(node->as.foreign_declaration.symbol_name.start_ptr + 1,
                  node->as.foreign_declaration.symbol_name.length - 2);
    char *ferro_fn_name =
        substring(node->as.foreign_declaration.fn_name.start_ptr,
                  node->as.foreign_declaration.fn_name.length);

    LLVMValueRef fn =
        LLVMAddFunction(llvm_module, source_name, signature.function_type);
    add_function_to_symbol_table(ferro_fn_name, fn);

    // Cleanup
    if (signature.param_types)
      free(signature.param_types);
    free(source_name);
    free(ferro_fn_name);
  } break;

  case AST_FUNCTION_DECLARATION: {
    FunctionSignature signature = create_function_signature(
        node->as.function_declaration.return_type,
        node->as.function_declaration.parameters, llvm_context, false);

    char *fn_name = substring(node->as.function_declaration.fn_name.start_ptr,
                              node->as.function_declaration.fn_name.length);
    LLVMValueRef fn =
        LLVMAddFunction(llvm_module, fn_name, signature.function_type);
    add_function_to_symbol_table(fn_name, fn);

    // Create function body
    LLVMBasicBlockRef fn_main =
        LLVMAppendBasicBlockInContext(llvm_context, fn, "entry");
    LLVMPositionBuilderAtEnd(builder, fn_main);

    // Convert statements to IR
    bool has_return = false;
    AstNode *block_node = node->as.function_declaration.block;
    for (int i = 0; i < (int)block_node->as.block_statement.statements.length;
         i++) {
      AstNode *stmt = block_node->as.block_statement.statements.data[i];

      if (stmt->kind == AST_RETURN_STATEMENT) {
        has_return = true;
        convert_statement(stmt, llvm_module, llvm_context, builder);
        break;
      } else {
        convert_statement(stmt, llvm_module, llvm_context, builder);
      }
    }

    // Add default return if none found
    if (!has_return) {
      LLVMTypeRef return_type = LLVMGetReturnType(signature.function_type);
      if (LLVMGetTypeKind(return_type) == LLVMVoidTypeKind) {
        LLVMBuildRetVoid(builder);
      } else {
        // A non-void function must return a value. Emit an error and exit.
        fprintf(stderr, "Error: Function must return a value of type ");
        char *type_str = LLVMPrintTypeToString(return_type);
        fprintf(stderr, "%s\n", type_str);
        LLVMDisposeMessage(type_str); // Free the type string memory
        exit(EXIT_FAILURE);
      }
    }

    // Cleanup
    if (signature.param_types)
      free(signature.param_types);
    free(fn_name);
  } break;
  default:
    fprintf(stderr, "Error: Unsupported AST declaration kind: %d\n",
            node->kind);
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

  // Process all declarations by calling convert_declaration
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