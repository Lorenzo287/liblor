ifeq ($(origin CC),default)
CC := clang
endif

CPPFLAGS ?= -Iinclude
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic

BUILD_DIR := .build
PYTHON ?= python
LIB_OBJS := $(BUILD_DIR)/arena.o
TEST_ARENA := $(BUILD_DIR)/test_arena$(EXE)
TEST_SINGLE_HEADER := $(BUILD_DIR)/test_single_header$(EXE)
TEST_SINGLE_HEADER_CUSTOM_PREFIX := $(BUILD_DIR)/test_single_header_custom_prefix$(EXE)
EXAMPLE_ARENA := $(BUILD_DIR)/arena_basic$(EXE)
SINGLE_HEADER := lor.h

ifeq ($(OS),Windows_NT)
EXE := .exe
else
EXE :=
endif

MKDIR_BUILD = mkdir -p $(BUILD_DIR)
RUN_TEST_ARENA = ./$(TEST_ARENA)
RM_BUILD = rm -rf $(BUILD_DIR)

.PHONY: all test example single-header clean

all: test example single-header

test: $(TEST_ARENA) $(TEST_SINGLE_HEADER) $(TEST_SINGLE_HEADER_CUSTOM_PREFIX)
	$(RUN_TEST_ARENA)
	./$(TEST_SINGLE_HEADER)
	./$(TEST_SINGLE_HEADER_CUSTOM_PREFIX)

example: $(EXAMPLE_ARENA)

single-header: $(SINGLE_HEADER)

$(BUILD_DIR):
	$(MKDIR_BUILD)

$(BUILD_DIR)/arena.o: src/arena.c include/lor/arena.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c src/arena.c -o $@

$(TEST_ARENA): tests/test_arena.c $(LIB_OBJS) include/lor/arena.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LIB_OBJS) tests/test_arena.c -o $@

$(TEST_SINGLE_HEADER): tests/test_single_header.c $(SINGLE_HEADER) | $(BUILD_DIR)
	$(CC) $(CFLAGS) tests/test_single_header.c -o $@

$(TEST_SINGLE_HEADER_CUSTOM_PREFIX): tests/test_single_header_custom_prefix.c $(SINGLE_HEADER) | $(BUILD_DIR)
	$(CC) $(CFLAGS) tests/test_single_header_custom_prefix.c -o $@

$(EXAMPLE_ARENA): examples/arena_basic.c $(LIB_OBJS) include/lor/lor.h include/lor/arena.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LIB_OBJS) examples/arena_basic.c -o $@

$(SINGLE_HEADER): tools/gen_single_header.py tools/lor_modules.json include/lor/arena.h src/arena.c
	$(PYTHON) tools/gen_single_header.py --output $(SINGLE_HEADER)

clean:
	$(RM_BUILD)
