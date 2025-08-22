# LLVM; HomeBrew configured paths.
LLVM_CONFIG  = /opt/homebrew/opt/llvm/bin/llvm-config
LLVM_PREFIX  = /opt/homebrew/opt/llvm
LLVM_INCLUDE = /opt/homebrew/Cellar/llvm/20.1.8/include
LLVM_LIB     = $(LLVM_PREFIX)/lib

CC   = clang
SRC ?= ./src/main.c
OUT  = ./build/main.o
LL   = ./build/main.ll

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

all: $(LL)

# Step 1: Compile the code generator into an object/executable
$(OUT): $(SRC)
	mkdir -p ./build
	$(CC) $(SRC) $(CFLAGS) $(LDFLAGS) -o $(OUT)

# Step 2: Run the code generator to produce main.ll
$(LL): $(OUT)
	./$(OUT) > $(LL)

clean:
	rm -f $(OUT) $(LL) 
