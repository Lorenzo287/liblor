ifeq ($(origin CC),default)
CC := clang
endif

CPPFLAGS ?= -Iinclude
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic

override BUILD_ROOT := .build
BUILD_PROFILE ?= default
override BUILD_DIR := $(BUILD_ROOT)/$(BUILD_PROFILE)
PYTHON ?= python

STRICT_CFLAGS := -std=c11 -Wall -Wextra -Wpedantic -Werror
RELEASE_CC ?= $(CC)
RELEASE_LTO ?=

CC_COMMAND := $(notdir $(firstword $(CC)))
ifneq (,$(findstring clang,$(CC_COMMAND)))
CC_PROFILE := clang
else ifneq (,$(findstring gcc,$(CC_COMMAND)))
CC_PROFILE := gcc
else
CC_PROFILE := $(CC_COMMAND)
endif

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

ifneq (,$(findstring clang,$(RELEASE_CC_COMMAND)))
ifeq ($(OS),Windows_NT)
DEFAULT_RELEASE_LTO := -flto -fuse-ld=lld
else
DEFAULT_RELEASE_LTO := -flto
endif
else
DEFAULT_RELEASE_LTO := -flto
endif

ifeq ($(OS),Windows_NT)
EXE := .exe
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
EXPORT_INPUTS := tools/gen_exports.py tools/lor_modules.json
RELEASE_STATIC_TEST := $(BUILD_DIR)/test_release_static$(EXE)
RELEASE_SHARED_TEST := $(BUILD_DIR)/test_release_shared$(EXE)

MKDIR_BUILD = mkdir -p $(BUILD_DIR)

.PHONY: all test example example-sh single-header clang gcc strict leakcheck \
	check release release-check release-lto release-lto-check release-verify \
	release-libraries clean

all: test example example-sh single-header

test: $(TEST_BINS)
	@for test in $(TEST_BINS); do ./$$test || exit $$?; done

example: $(EXAMPLE_BINS)

example-sh: $(EXAMPLE_SH_BINS)

single-header: $(SINGLE_HEADER)

clang:
	$(MAKE) BUILD_PROFILE=clang CC=clang all

gcc:
	$(MAKE) BUILD_PROFILE=gcc CC=gcc all

strict:
	$(MAKE) BUILD_PROFILE=clang-strict CC=clang CFLAGS="$(STRICT_CFLAGS)" all

leakcheck:
	$(MAKE) BUILD_PROFILE=clang-leakcheck CC=clang \
		CPPFLAGS="$(CPPFLAGS) -DLOR_LEAKCHECK" CFLAGS="$(STRICT_CFLAGS)" all

check:
	$(MAKE) strict
	$(MAKE) BUILD_PROFILE=gcc-strict CC=gcc CFLAGS="$(STRICT_CFLAGS)" all
	$(MAKE) leakcheck

release:
	$(MAKE) BUILD_PROFILE="$(RELEASE_PROFILE)" CC="$(RELEASE_CC)" \
		CFLAGS="$(RELEASE_CFLAGS)" release-libraries

release-check:
	$(MAKE) BUILD_PROFILE="$(RELEASE_PROFILE)" CC="$(RELEASE_CC)" \
		CFLAGS="$(RELEASE_CFLAGS)" release-verify

release-lto:
	$(MAKE) RELEASE_LTO="$(DEFAULT_RELEASE_LTO)" release

release-lto-check:
	$(MAKE) RELEASE_LTO="$(DEFAULT_RELEASE_LTO)" release-check

release-verify: test $(RELEASE_STATIC_TEST) $(RELEASE_SHARED_TEST)
	./$(RELEASE_STATIC_TEST)
	./$(RELEASE_SHARED_TEST)

release-libraries: $(STATIC_LIB) $(SHARED_LIB)
	@echo "static library: $(STATIC_LIB)"
	@echo "shared library: $(SHARED_LIB)"
ifneq ($(IMPORT_LIB),)
	@echo "import library: $(IMPORT_LIB)"
endif

$(BUILD_DIR):
	$(MKDIR_BUILD)

$(BUILD_DIR)/%.o: src/%.c $(PUBLIC_HEADERS) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(STATIC_LIB): $(LIB_OBJS) | $(BUILD_DIR)
	$(RELEASE_ARCHIVE)

ifeq ($(OS),Windows_NT)
$(EXPORT_DEF): $(EXPORT_INPUTS) | $(BUILD_DIR)
	$(PYTHON) tools/gen_exports.py --output $@

$(SHARED_LIB): $(LIB_OBJS) $(EXPORT_DEF) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(SHARED_LINK_FLAGS) $(LIB_OBJS) \
		$(SHARED_EXPORT_INPUT) -o $@
else
$(SHARED_LIB): $(LIB_OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(SHARED_LINK_FLAGS) $(LIB_OBJS) -o $@
endif

$(RELEASE_STATIC_TEST): tests/test_random.c $(STATIC_LIB) $(PUBLIC_HEADERS) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(STATIC_LIB) -o $@

$(RELEASE_SHARED_TEST): tests/test_random.c $(SHARED_LIB) $(PUBLIC_HEADERS) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(SHARED_TEST_LINK) -o $@

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
	rm -rf .build
