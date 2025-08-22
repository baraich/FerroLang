#include "llvm-c/Types.h"
#include <_stdio.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>
#include <llvm-c/Core.h>
#include <stdio.h>

int main() {
  // Setting up LLVM context and module.
  LLVMContextRef llvm_context = LLVMContextCreate();
  LLVMModuleRef llvm_module = LLVMModuleCreateWithNameInContext("main_module", llvm_context);

  // Setting up i8 and function type. 
  LLVMTypeRef i8 = LLVMInt8TypeInContext(llvm_context);
  LLVMTypeRef fn_type = LLVMFunctionType(i8, NULL, 0, 0);

  // Defining a function declaration.
  LLVMValueRef fn = LLVMAddFunction(llvm_module, "main", fn_type);

  // IR Generation setup.
  LLVMBasicBlockRef main_block = LLVMAppendBasicBlockInContext(llvm_context, fn, "main");
  LLVMBuilderRef builder = LLVMCreateBuilderInContext(llvm_context);
  LLVMPositionBuilderAtEnd(builder, main_block);

  // Adding a return statement to the function.
  LLVMValueRef zero = LLVMConstInt(i8, 0, 0);
  LLVMBuildRet(builder, zero);

  // Verifying module
  char *err = NULL;
  if (LLVMVerifyModule(llvm_module, LLVMReturnStatusAction, &err)) {
    // Displaying the error message to the console.
    fprintf(stderr, "Unable to verify the module: %s\n", err);

    // Cleanup
    LLVMDisposeMessage(err);
    LLVMDisposeBuilder(builder);
    LLVMDisposeModule(llvm_module);
    LLVMContextDispose(llvm_context);
    return 1;
  }
  if (err) LLVMDisposeMessage(err);

  // Printing IR to the console.
  char *ir = LLVMPrintModuleToString(llvm_module);
  puts(ir);

  // Cleanup
  LLVMDisposeMessage(ir);
  LLVMDisposeBuilder(builder);
  LLVMDisposeModule(llvm_module);
  LLVMContextDispose(llvm_context);

  return 0;
}