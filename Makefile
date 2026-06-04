ifeq ($(origin CC),default)
CC := clang
endif

CPPFLAGS ?= -Iinclude
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic

BUILD_DIR := .build
PYTHON ?= python

ifeq ($(OS),Windows_NT)
EXE := .exe
else
EXE :=
endif

LIB_SRCS := $(wildcard src/*.c)
PUBLIC_HEADERS := $(wildcard include/lor/*.h)
LIB_OBJS := $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(LIB_SRCS))

TEST_SRCS := $(wildcard tests/test_*.c)
SINGLE_HEADER_TEST_SRCS := $(wildcard tests/test_single_header*.c)
LIB_TEST_SRCS := $(filter-out $(SINGLE_HEADER_TEST_SRCS),$(TEST_SRCS))
LIB_TEST_BINS := $(patsubst tests/%.c,$(BUILD_DIR)/%$(EXE),$(LIB_TEST_SRCS))
SINGLE_HEADER_TEST_BINS := $(patsubst tests/%.c,$(BUILD_DIR)/%$(EXE),$(SINGLE_HEADER_TEST_SRCS))
TEST_BINS := $(LIB_TEST_BINS) $(SINGLE_HEADER_TEST_BINS)

EXAMPLE_SRCS := $(wildcard examples/*.c)
EXAMPLE_SH_SRCS := $(wildcard examples_sh/*.c)
EXAMPLE_BINS := $(patsubst examples/%.c,$(BUILD_DIR)/example_%$(EXE),$(EXAMPLE_SRCS))
EXAMPLE_SH_BINS := $(patsubst examples_sh/%.c,$(BUILD_DIR)/example_sh_%$(EXE),$(EXAMPLE_SH_SRCS))
SINGLE_HEADER := lor.h
SINGLE_HEADER_INPUTS := tools/gen_single_header.py tools/lor_modules.json $(PUBLIC_HEADERS) $(LIB_SRCS)

MKDIR_BUILD = mkdir -p $(BUILD_DIR)
RM_BUILD = rm -rf $(BUILD_DIR)

.PHONY: all test example example-sh single-header clean

all: test example example-sh single-header

test: $(TEST_BINS)
	@for test in $(TEST_BINS); do ./$$test || exit $$?; done

example: $(EXAMPLE_BINS)

example-sh: $(EXAMPLE_SH_BINS)

single-header: $(SINGLE_HEADER)

$(BUILD_DIR):
	$(MKDIR_BUILD)

$(BUILD_DIR)/%.o: src/%.c $(PUBLIC_HEADERS) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(SINGLE_HEADER_TEST_BINS): $(BUILD_DIR)/%$(EXE): tests/%.c $(SINGLE_HEADER) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(LIB_TEST_BINS): $(BUILD_DIR)/%$(EXE): tests/%.c $(LIB_OBJS) $(PUBLIC_HEADERS) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LIB_OBJS) $< -o $@

$(EXAMPLE_BINS): $(BUILD_DIR)/example_%$(EXE): examples/%.c $(LIB_OBJS) $(PUBLIC_HEADERS) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LIB_OBJS) $< -o $@

$(EXAMPLE_SH_BINS): $(BUILD_DIR)/example_sh_%$(EXE): examples_sh/%.c $(SINGLE_HEADER) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(SINGLE_HEADER): $(SINGLE_HEADER_INPUTS)
	$(PYTHON) tools/gen_single_header.py --output $(SINGLE_HEADER)

clean:
	$(RM_BUILD)
