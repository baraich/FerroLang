# LLVM; HomeBrew configured paths.
LLVM_CONFIG  = /opt/homebrew/opt/llvm/bin/llvm-config
LLVM_PREFIX  = /opt/homebrew/opt/llvm
LLVM_INCLUDE = /opt/homebrew/Cellar/llvm/20.1.8/include
LLVM_LIB     = $(LLVM_PREFIX)/lib

CC    = clang
SRC  ?= ./src/main.c
OUT   = ./build/compiler       # FerroLang compiler binary
LL    = ./build/main.ll        # LLVM IR output
BIN   = ./build/main           # Final runnable binary

# Standard library C sources
STDLIB_C = ./std/io.c

# Compilation flags from .clangd + llvm-config
CFLAGS = -Wall -Wextra \
         -I$(LLVM_INCLUDE) \
         -D__STDC_CONSTANT_MACROS \
         -D__STDC_FORMAT_MACROS \
         -D__STDC_LIMIT_MACROS \
         $(shell $(LLVM_CONFIG) --cflags)

# Link against LLVM libs for codegen
LDFLAGS = -L$(LLVM_LIB) \
          $(shell $(LLVM_CONFIG) --system-libs --libs core analysis bitwriter) \
          -Wl,-rpath,$(LLVM_LIB)

all: $(BIN)

# Step 1: Build the FerroLang compiler itself
$(OUT): $(SRC)
	mkdir -p ./build
	$(CC) $(SRC) $(CFLAGS) $(LDFLAGS) -o $(OUT)

# Step 2: Use compiler to generate LLVM IR from FerroLang source
$(LL): $(OUT) ./testing/main.fl
	./$(OUT) > $(LL)

# Step 3: Compile LLVM IR + stdlib into a runnable binary
$(BIN): $(LL) $(STDLIB_C)
	$(CC) $(LL) $(STDLIB_C) -o $(BIN)

# Clean everything
clean:
	rm -rf ./build
