ifeq ($(origin CC),default)
CC := clang
endif
ifeq ($(origin CXX),default)
CXX := clang++
endif

.DEFAULT_GOAL := all

CPPFLAGS ?= -Iinclude
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic

override BUILD_ROOT := .build
BUILD_PROFILE ?= default
override BUILD_DIR := $(BUILD_ROOT)/$(BUILD_PROFILE)
PYTHON ?= python

STRICT_CFLAGS := -std=c11 -Wall -Wextra -Wpedantic -Werror
STRICT_CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -Werror
RELEASE_CC ?= $(CC)
RELEASE_LTO ?=

CC_COMMAND := $(notdir $(firstword $(CC)))

RELEASE_CC_COMMAND := $(notdir $(firstword $(RELEASE_CC)))
ifneq (,$(findstring clang,$(RELEASE_CC_COMMAND)))
RELEASE_CC_PROFILE := clang
else ifneq (,$(findstring gcc,$(RELEASE_CC_COMMAND)))
RELEASE_CC_PROFILE := gcc
else
RELEASE_CC_PROFILE := $(RELEASE_CC_COMMAND)
endif
ifneq ($(strip $(RELEASE_LTO)),)
RELEASE_CC_PROFILE := $(RELEASE_CC_PROFILE)-lto
endif
RELEASE_PROFILE := release/$(RELEASE_CC_PROFILE)

ifeq ($(OS),Windows_NT)
EXE := .exe
THREAD_FLAGS :=
PIC_CFLAGS :=
SHARED_LIB := $(BUILD_DIR)/lor.dll
EXPORT_DEF := $(BUILD_DIR)/lor.def
ifneq (,$(findstring clang,$(CC_COMMAND)))
STATIC_LIB := $(BUILD_DIR)/lor_static.lib
IMPORT_LIB := $(BUILD_DIR)/lor.lib
RELEASE_ARCHIVE = llvm-lib /nologo /out:$@ $(LIB_OBJS)
SHARED_LINK_FLAGS = -shared -Xlinker /def:$(EXPORT_DEF) \
	-Xlinker /implib:$(IMPORT_LIB)
SHARED_EXPORT_INPUT :=
else
STATIC_LIB := $(BUILD_DIR)/liblor.a
IMPORT_LIB := $(BUILD_DIR)/liblor.dll.a
RELEASE_ARCHIVE = $(AR) rcs $@ $(LIB_OBJS)
SHARED_LINK_FLAGS = -shared -Wl,--out-implib,$(IMPORT_LIB)
SHARED_EXPORT_INPUT := $(EXPORT_DEF)
endif
SHARED_TEST_LINK = $(IMPORT_LIB)
else
EXE :=
THREAD_FLAGS := -pthread
PIC_CFLAGS := -fPIC
STATIC_LIB := $(BUILD_DIR)/liblor.a
IMPORT_LIB :=
RELEASE_ARCHIVE = $(AR) rcs $@ $(LIB_OBJS)
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
SHARED_LIB := $(BUILD_DIR)/liblor.dylib
SHARED_LINK_FLAGS := -dynamiclib
SHARED_TEST_LINK = -L$(BUILD_DIR) -llor -Wl,-rpath,@loader_path
else
SHARED_LIB := $(BUILD_DIR)/liblor.so
SHARED_LINK_FLAGS := -shared
SHARED_TEST_LINK = -L$(BUILD_DIR) -llor -Wl,-rpath,'$$ORIGIN'
endif
endif

RELEASE_CFLAGS := -std=c11 -O3 -DNDEBUG -Wall -Wextra -Wpedantic \
	$(PIC_CFLAGS) $(RELEASE_LTO)

LIB_SRCS := $(wildcard src/*.c)
PUBLIC_HEADERS := $(wildcard include/lor/*.h)
LIB_OBJS := $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(LIB_SRCS))

TEST_SRCS := $(wildcard tests/test_*.c)
TEST_HEADERS := $(wildcard tests/*.h)
SINGLE_HEADER_TEST_SRCS := $(wildcard tests/test_single_header*.c)
TRACE_AUTO_TEST_SRC := tests/test_trace_auto.c
LIB_TEST_SRCS := $(filter-out $(SINGLE_HEADER_TEST_SRCS) $(TRACE_AUTO_TEST_SRC),$(TEST_SRCS))
LIB_TEST_BINS := $(patsubst tests/%.c,$(BUILD_DIR)/%$(EXE),$(LIB_TEST_SRCS))
SINGLE_HEADER_TEST_BINS := $(patsubst tests/%.c,$(BUILD_DIR)/%$(EXE),$(SINGLE_HEADER_TEST_SRCS))
TRACE_AUTO_TEST_BIN := $(BUILD_DIR)/test_trace_auto$(EXE)
TEST_BINS := $(LIB_TEST_BINS) $(SINGLE_HEADER_TEST_BINS) $(TRACE_AUTO_TEST_BIN)
CPP_TEST_SRCS := $(wildcard tests/test_*.cpp)
CPP_TEST_BINS := $(patsubst tests/%.cpp,$(BUILD_DIR)/%$(EXE),$(CPP_TEST_SRCS))

EXAMPLE_SRCS := $(wildcard examples/*.c)
EXAMPLE_SH_SRCS := $(wildcard examples_sh/*.c)
EXAMPLE_BINS := $(patsubst examples/%.c,$(BUILD_DIR)/example_%$(EXE),$(EXAMPLE_SRCS))
EXAMPLE_SH_BINS := $(patsubst examples_sh/%.c,$(BUILD_DIR)/example_sh_%$(EXE),$(EXAMPLE_SH_SRCS))
SINGLE_HEADER := lor.h
SINGLE_HEADER_INPUTS := tools/gen_single_header.py tools/lor_modules.json $(PUBLIC_HEADERS) $(LIB_SRCS)
EXPORT_INPUTS := tools/gen_exports.py tools/lor_modules.json
RELEASE_TEST_SRC := tests/test_print.c
RELEASE_STATIC_TEST := $(BUILD_DIR)/test_release_static$(EXE)
RELEASE_SHARED_TEST := $(BUILD_DIR)/test_release_shared$(EXE)

MKDIR_BUILD = mkdir -p $(BUILD_DIR)

.PHONY: all test examples single-header gcc leakcheck check release \
	release-check _release-libraries _release-verify clean

all: $(LIB_OBJS) single-header

test: $(TEST_BINS) $(CPP_TEST_BINS)
	@for test in $(TEST_BINS); do ./$$test || exit $$?; done
	@for test in $(CPP_TEST_BINS); do ./$$test || exit $$?; done

examples: $(EXAMPLE_BINS) $(EXAMPLE_SH_BINS)

single-header: $(SINGLE_HEADER)

gcc:
	$(MAKE) BUILD_PROFILE=gcc CC=gcc CXX=g++ all

leakcheck:
	$(MAKE) BUILD_PROFILE=clang-leakcheck CC=clang \
		CPPFLAGS="$(CPPFLAGS) -DLOR_LEAKCHECK" CFLAGS="$(STRICT_CFLAGS)" test

check:
	$(MAKE) BUILD_PROFILE=clang-strict CC=clang CXX=clang++ \
		CFLAGS="$(STRICT_CFLAGS)" CXXFLAGS="$(STRICT_CXXFLAGS)" \
		test examples
	$(MAKE) BUILD_PROFILE=gcc-strict CC=gcc CXX=g++ \
		CFLAGS="$(STRICT_CFLAGS)" CXXFLAGS="$(STRICT_CXXFLAGS)" \
		test examples
	$(MAKE) leakcheck

release:
	$(MAKE) BUILD_PROFILE="$(RELEASE_PROFILE)" CC="$(RELEASE_CC)" \
		CFLAGS="$(RELEASE_CFLAGS)" _release-libraries

release-check:
	$(MAKE) BUILD_PROFILE="$(RELEASE_PROFILE)" CC="$(RELEASE_CC)" \
		CFLAGS="$(RELEASE_CFLAGS)" _release-verify

_release-verify: test $(RELEASE_STATIC_TEST) $(RELEASE_SHARED_TEST)
	./$(RELEASE_STATIC_TEST)
	./$(RELEASE_SHARED_TEST)

_release-libraries: $(STATIC_LIB) $(SHARED_LIB)
	@echo "static library: $(STATIC_LIB)"
	@echo "shared library: $(SHARED_LIB)"
ifneq ($(IMPORT_LIB),)
	@echo "import library: $(IMPORT_LIB)"
endif

$(BUILD_DIR):
	$(MKDIR_BUILD)

$(BUILD_DIR)/%.o: src/%.c $(PUBLIC_HEADERS) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(THREAD_FLAGS) -c $< -o $@

$(STATIC_LIB): $(LIB_OBJS) | $(BUILD_DIR)
	$(RELEASE_ARCHIVE)

ifeq ($(OS),Windows_NT)
$(EXPORT_DEF): $(EXPORT_INPUTS) | $(BUILD_DIR)
	$(PYTHON) tools/gen_exports.py --output $@

$(SHARED_LIB): $(LIB_OBJS) $(EXPORT_DEF) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(THREAD_FLAGS) $(SHARED_LINK_FLAGS) $(LIB_OBJS) \
		$(SHARED_EXPORT_INPUT) -o $@
else
$(SHARED_LIB): $(LIB_OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(THREAD_FLAGS) $(SHARED_LINK_FLAGS) $(LIB_OBJS) -o $@
endif

$(RELEASE_STATIC_TEST): $(RELEASE_TEST_SRC) $(STATIC_LIB) $(PUBLIC_HEADERS) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(THREAD_FLAGS) $< $(STATIC_LIB) -o $@

$(RELEASE_SHARED_TEST): $(RELEASE_TEST_SRC) $(SHARED_LIB) $(PUBLIC_HEADERS) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(THREAD_FLAGS) $< $(SHARED_TEST_LINK) -o $@

$(SINGLE_HEADER_TEST_BINS): $(BUILD_DIR)/%$(EXE): tests/%.c $(TEST_HEADERS) $(SINGLE_HEADER) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(THREAD_FLAGS) $< -o $@

$(LIB_TEST_BINS): $(BUILD_DIR)/%$(EXE): tests/%.c $(TEST_HEADERS) $(LIB_OBJS) $(PUBLIC_HEADERS) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(THREAD_FLAGS) $(LIB_OBJS) $< -o $@

ifeq ($(OS),Windows_NT)
TRACE_AUTO_LIBS := -ldbghelp
else
TRACE_AUTO_LIBS :=
endif

$(TRACE_AUTO_TEST_BIN): $(TRACE_AUTO_TEST_SRC) src/trace.c $(TEST_HEADERS) $(PUBLIC_HEADERS) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(THREAD_FLAGS) -DLOR_TRACE_AUTO \
		-finstrument-functions src/trace.c $< $(TRACE_AUTO_LIBS) -o $@

$(CPP_TEST_BINS): $(BUILD_DIR)/%$(EXE): tests/%.cpp $(LIB_OBJS) $(PUBLIC_HEADERS) $(SINGLE_HEADER) | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(THREAD_FLAGS) $(LIB_OBJS) $< -o $@

$(EXAMPLE_BINS): $(BUILD_DIR)/example_%$(EXE): examples/%.c $(LIB_OBJS) $(PUBLIC_HEADERS) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(THREAD_FLAGS) $(LIB_OBJS) $< -o $@

$(EXAMPLE_SH_BINS): $(BUILD_DIR)/example_sh_%$(EXE): examples_sh/%.c $(SINGLE_HEADER) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(THREAD_FLAGS) $< -o $@

$(SINGLE_HEADER): $(SINGLE_HEADER_INPUTS)
	$(PYTHON) tools/gen_single_header.py --output $(SINGLE_HEADER)

clean:
	rm -rf .build
