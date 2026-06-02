ifeq ($(origin CC),default)
CC := clang
endif

CPPFLAGS ?= -Iinclude
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic

BUILD_DIR := .build
LIB_OBJS := $(BUILD_DIR)/arena.o
TEST_ARENA := $(BUILD_DIR)/test_arena$(EXE)
EXAMPLE_ARENA := $(BUILD_DIR)/arena_basic$(EXE)

ifeq ($(OS),Windows_NT)
EXE := .exe
else
EXE :=
endif

MKDIR_BUILD = mkdir -p $(BUILD_DIR)
RUN_TEST_ARENA = ./$(TEST_ARENA)
RM_BUILD = rm -rf $(BUILD_DIR)

.PHONY: all test example clean format

all: test example

test: $(TEST_ARENA)
	$(RUN_TEST_ARENA)

example: $(EXAMPLE_ARENA)

$(BUILD_DIR):
	$(MKDIR_BUILD)

$(BUILD_DIR)/arena.o: src/arena.c include/lor/arena.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c src/arena.c -o $@

$(TEST_ARENA): tests/test_arena.c $(LIB_OBJS) include/lor/arena.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LIB_OBJS) tests/test_arena.c -o $@

$(EXAMPLE_ARENA): examples/arena_basic.c $(LIB_OBJS) include/lor/lor.h include/lor/arena.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LIB_OBJS) examples/arena_basic.c -o $@

format:
	clang-format -i include/lor/arena.h include/lor/lor.h src/arena.c tests/test_arena.c examples/arena_basic.c

clean:
	$(RM_BUILD)
