# UltraCPP C compiler - Makefile (Phase 1: lexer)
#
# Build:
#   make            -> build bin/uc_lexer
#   make test       -> run unit tests
#   make clean      -> remove build artifacts
#   make distclean  -> remove build/ directory entirely

CC      ?= cc
CSTD    ?= -std=c99
WARN    ?= -Wall -Wextra -Wpedantic -Wshadow -Wstrict-prototypes -Wmissing-prototypes
OPT     ?= -O2 -g
CFLAGS  ?= $(CSTD) $(WARN) $(OPT) -Iinclude
LDFLAGS ?=

BUILD_DIR := build
SRC_DIR   := src
INC_DIR   := include
TEST_DIR  := tests
BIN       := $(BUILD_DIR)/uc_lexer

SOURCES  := $(wildcard $(SRC_DIR)/*.c)
OBJECTS  := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SOURCES))

# Library object files: everything except main.o (so tests can link without
# colliding with their own main()).
LIB_OBJECTS := $(filter-out $(BUILD_DIR)/main.o,$(OBJECTS))

TEST_SOURCES := $(wildcard $(TEST_DIR)/*.c)
TEST_BINS    := $(patsubst $(TEST_DIR)/%.c,$(BUILD_DIR)/%,$(TEST_SOURCES))

.PHONY: all test clean distclean help

all: $(BIN)

$(BIN): $(OBJECTS) | $(BUILD_DIR)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%: $(TEST_DIR)/%.c $(LIB_OBJECTS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< $(LIB_OBJECTS) -o $@ $(LDFLAGS)

$(BUILD_DIR):
	@mkdir -p $@

test: $(TEST_BINS)
	@set -e; for t in $(TEST_BINS); do \
		echo "==> running $$t"; \
		$$t; \
	done

clean:
	rm -rf $(BUILD_DIR)

distclean: clean

help:
	@echo "UltraCPP C compiler - Makefile (Phase 1: lexer)"
	@echo ""
	@echo "Targets:"
	@echo "  all          Build bin/uc_lexer (default)"
	@echo "  test         Run all unit tests"
	@echo "  clean        Remove build artifacts"
	@echo "  distclean    Remove entire build directory"
	@echo "  help         Print this help"