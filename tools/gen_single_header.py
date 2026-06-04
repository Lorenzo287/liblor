#!/usr/bin/env python3
# SPDX-License-Identifier: MIT

from __future__ import annotations

import argparse
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_MANIFEST = ROOT / "tools" / "lor_modules.json"
DEFAULT_OUTPUT = ROOT / "lor.h"


def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def write_text(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8", newline="\n")


def strip_spdx(lines: list[str]) -> list[str]:
    return [
        line for line in lines if not line.strip().startswith("// SPDX-License-Identifier:")
    ]


def strip_local_includes(lines: list[str]) -> list[str]:
    result = []
    for line in lines:
        stripped = line.strip()
        if stripped.startswith('#include "lor/') or stripped == '#include "lor.h"':
            continue
        if stripped in {
            "#define LOR_MEMORY_NO_STDLIB_MACROS",
            "#define LOR_MEMORY_NO_LOCATION_MACROS",
            "#define LOR_MEMORY_INTERNAL",
        }:
            continue
        result.append(line)
    return result


def strip_header_guard(lines: list[str], guard: str) -> list[str]:
    result = []
    last_nonempty = -1

    for i, line in enumerate(lines):
        if line.strip():
            last_nonempty = i

    for i, line in enumerate(lines):
        stripped = line.strip()
        if stripped == f"#ifndef {guard}" or stripped == f"#define {guard}":
            continue
        if i == last_nonempty and stripped == "#endif":
            continue
        result.append(line)

    return result


def strip_single_header_late_macros(lines: list[str]) -> list[str]:
    result = []
    skipping = False

    for line in lines:
        stripped = line.strip()

        if stripped.startswith("/* Optional macro mode."):
            skipping = True
            continue

        if skipping:
            if stripped == "#endif":
                skipping = False
            continue

        result.append(line)

    return result


def clean_header(path: Path) -> str:
    guard = path.stem.upper().replace("-", "_") + "_H"
    if path.stem == "lor":
        guard = "LOR_H"
    else:
        guard = f"LOR_{path.stem.upper()}_H"

    lines = read_text(path).splitlines()
    lines = strip_spdx(lines)
    lines = strip_local_includes(lines)
    lines = strip_single_header_late_macros(lines)
    lines = strip_header_guard(lines, guard)
    return "\n".join(lines).strip() + "\n"


def clean_source(path: Path) -> str:
    lines = read_text(path).splitlines()
    lines = strip_spdx(lines)
    lines = strip_local_includes(lines)
    return "\n".join(lines).strip() + "\n"


def module_enabled_condition(module: dict) -> str:
    return module["enable_macro"]


def function_suffix(alias: str) -> str:
    return alias


def emit_module_selection(modules: list[dict]) -> str:
    enable_macros = [module["enable_macro"] for module in modules]
    no_modules = " && ".join(f"!defined({macro})" for macro in enable_macros)

    out = []
    out.append("/* Module selection")
    out.append("   If no LOR_ENABLE_* macro is defined, every stable module is enabled.")
    out.append("   Define one or more LOR_ENABLE_* macros before including this header")
    out.append("   to include only those modules. */")
    out.append(f"#if {no_modules}")
    out.append("#define LOR_ENABLE_ALL")
    out.append("#endif")
    out.append("")
    out.append("#ifdef LOR_ENABLE_ALL")
    for macro in enable_macros:
        out.append(f"#define {macro}")
    out.append("#endif")
    return "\n".join(out) + "\n"


def emit_custom_prefix(modules: list[dict]) -> str:
    out = []
    out.append("/* Optional compiled function prefix")
    out.append("   Example: #define LOR_CUSTOM_PREFIX my_")
    out.append("   turns lor_arena_init_config into my_arena_init_config in this translation unit.")
    out.append("   This affects declarations and definitions, so all translation units")
    out.append("   using the generated header must use the same custom prefix. */")
    out.append("#ifdef LOR_CUSTOM_PREFIX")
    out.append("#define LOR__JOIN2(a, b) a##b")
    out.append("#define LOR__JOIN(a, b) LOR__JOIN2(a, b)")
    for module in modules:
        out.append(f"#ifdef {module_enabled_condition(module)}")
        for canonical, alias in module["symbols"].get("functions", []):
            out.append(f"#define {canonical} LOR__JOIN(LOR_CUSTOM_PREFIX, {function_suffix(alias)})")
        out.append("#endif")
    out.append("#endif")
    return "\n".join(out) + "\n"


def emit_declarations(modules: list[dict]) -> str:
    out = []
    for module in modules:
        path = ROOT / module["header"]
        out.append(f"/* === {module['name']}: declarations === */")
        out.append(f"#ifdef {module_enabled_condition(module)}")
        out.append(clean_header(path).rstrip())
        out.append("#endif")
        out.append("")
    return "\n".join(out)


def emit_implementations(modules: list[dict]) -> str:
    out = []
    out.append("#ifdef LOR_IMPLEMENTATION")
    out.append("")
    for module in modules:
        path = ROOT / module["source"]
        out.append(f"/* === {module['name']}: implementation === */")
        out.append(f"#ifdef {module_enabled_condition(module)}")
        out.append(clean_source(path).rstrip())
        out.append("#endif")
        out.append("")
    out.append("#endif")
    return "\n".join(out) + "\n"


def emit_strip_prefix_aliases(modules: list[dict]) -> str:
    out = []
    out.append("/* Optional short-name aliases")
    out.append("   These are preprocessor aliases only. They do not change compiled")
    out.append("   symbol names unless LOR_CUSTOM_PREFIX is also used. */")
    out.append("#ifdef LOR_STRIP_PREFIX")
    for module in modules:
        out.append(f"#ifdef {module_enabled_condition(module)}")
        for group in ("types", "macros", "functions"):
            for canonical, alias in module["symbols"].get(group, []):
                out.append(f"#define {alias} {canonical}")
        out.append("#endif")
    out.append("#endif")
    return "\n".join(out) + "\n"


def emit_late_macros() -> str:
    out = []
    out.append("/* Optional macro mode")
    out.append("   Provides designated-argument convenience wrappers after")
    out.append("   the implementation body has been emitted. */")
    out.append("#if !defined(LOR_MEMORY_INTERNAL) && !defined(LOR_MEMORY_NO_OPTION_MACROS)")
    out.append("#define LOR_MEMORY_SELECT_INIT_(_1, _2, _3, _4, _5, _6, NAME, ...) NAME")
    out.append("#define LOR_MEMORY_ARENA_INIT_DEFAULT_(arena) \\")
    out.append("    lor_arena_init_config((arena), NULL)")
    out.append("#define LOR_MEMORY_ARENA_INIT_OPTIONS_(arena, ...) \\")
    out.append("    lor_arena_init_config((arena), &(LorArenaConfig){__VA_ARGS__})")
    out.append("#define LOR_MEMORY_ARENA_INIT_DEBUG_DEFAULT_(arena) \\")
    out.append("    lor_arena_init_config_debug((arena), NULL, __FILE__, __LINE__)")
    out.append("#define LOR_MEMORY_ARENA_INIT_DEBUG_OPTIONS_(arena, ...) \\")
    out.append("    lor_arena_init_config_debug((arena), &(LorArenaConfig){__VA_ARGS__}, \\")
    out.append("                                __FILE__, __LINE__)")
    out.append("")
    out.append("#if defined(LOR_LEAKCHECK) && !defined(LOR_MEMORY_NO_LOCATION_MACROS)")
    out.append("#define LOR_MEMORY_ARENA_INIT_DEFAULT LOR_MEMORY_ARENA_INIT_DEBUG_DEFAULT_")
    out.append("#define LOR_MEMORY_ARENA_INIT_OPTIONS LOR_MEMORY_ARENA_INIT_DEBUG_OPTIONS_")
    out.append("#else")
    out.append("#define LOR_MEMORY_ARENA_INIT_DEFAULT LOR_MEMORY_ARENA_INIT_DEFAULT_")
    out.append("#define LOR_MEMORY_ARENA_INIT_OPTIONS LOR_MEMORY_ARENA_INIT_OPTIONS_")
    out.append("#endif")
    out.append("")
    out.append("#define LOR_MEMORY_ARENA_INIT_(...)                                      \\")
    out.append("    LOR_MEMORY_SELECT_INIT_(__VA_ARGS__, LOR_MEMORY_ARENA_INIT_OPTIONS,   \\")
    out.append("                            LOR_MEMORY_ARENA_INIT_OPTIONS,                \\")
    out.append("                            LOR_MEMORY_ARENA_INIT_OPTIONS,                \\")
    out.append("                            LOR_MEMORY_ARENA_INIT_OPTIONS,                \\")
    out.append("                            LOR_MEMORY_ARENA_INIT_OPTIONS,                \\")
    out.append("                            LOR_MEMORY_ARENA_INIT_DEFAULT, unused)        \\")
    out.append("    (__VA_ARGS__)")
    out.append("")
    out.append("/** Initializes an arena from optional designated configuration arguments.")
    out.append(" *")
    out.append(" * Supported forms:")
    out.append(" * `lor_arena_init(&arena)`")
    out.append(" * `lor_arena_init(&arena, .block_size = 128)`")
    out.append(" * `lor_arena_init(&arena, .backend = LOR_ARENA_BACKEND_VIRTUAL)`")
    out.append(" */")
    out.append("#undef lor_arena_init")
    out.append("#define lor_arena_init(...) LOR_MEMORY_ARENA_INIT_(__VA_ARGS__)")
    out.append("")
    out.append("#define LOR_MEMORY_SELECT_ALLOC_(_1, _2, _3, _4, _5, _6, NAME, ...) NAME")
    out.append("#define LOR_MEMORY_ARENA_ALLOC_DEFAULT_(arena, size) \\")
    out.append("    lor_arena_alloc_ex((arena), (size), (LorArenaAllocOptions)LOR_ARENA_ALLOC_OPTIONS_INIT)")
    out.append("#define LOR_MEMORY_ARENA_ALLOC_OPTIONS_(arena, size, ...) \\")
    out.append("    lor_arena_alloc_ex((arena), (size), (LorArenaAllocOptions){__VA_ARGS__})")
    out.append("")
    out.append("#define LOR_MEMORY_ARENA_ALLOC_(...)                                      \\")
    out.append("    LOR_MEMORY_SELECT_ALLOC_(__VA_ARGS__, LOR_MEMORY_ARENA_ALLOC_OPTIONS_, \\")
    out.append("                             LOR_MEMORY_ARENA_ALLOC_OPTIONS_,              \\")
    out.append("                             LOR_MEMORY_ARENA_ALLOC_OPTIONS_,              \\")
    out.append("                             LOR_MEMORY_ARENA_ALLOC_OPTIONS_,              \\")
    out.append("                             LOR_MEMORY_ARENA_ALLOC_DEFAULT_, unused)      \\")
    out.append("    (__VA_ARGS__)")
    out.append("")
    out.append("/** Allocates from an arena with optional designated allocation arguments.")
    out.append(" *")
    out.append(" * Supported forms:")
    out.append(" * `lor_arena_alloc(&arena, size)`")
    out.append(" * `lor_arena_alloc(&arena, size, .zero = true)`")
    out.append(" */")
    out.append("#undef lor_arena_alloc")
    out.append("#define lor_arena_alloc(...) LOR_MEMORY_ARENA_ALLOC_(__VA_ARGS__)")
    out.append("")
    out.append("#define LOR_MEMORY_SELECT_ALLOC_ARRAY_(_1, _2, _3, _4, _5, _6, NAME, ...) NAME")
    out.append("#define LOR_MEMORY_ARENA_ALLOC_ARRAY_DEFAULT_(arena, count, elem_size)       \\")
    out.append("    lor_arena_alloc_array_ex((arena), (count), (elem_size),                  \\")
    out.append("                             (LorArenaAllocOptions)LOR_ARENA_ALLOC_OPTIONS_INIT)")
    out.append("#define LOR_MEMORY_ARENA_ALLOC_ARRAY_OPTIONS_(arena, count, elem_size, ...) \\")
    out.append("    lor_arena_alloc_array_ex((arena), (count), (elem_size),                 \\")
    out.append("                             (LorArenaAllocOptions){__VA_ARGS__})")
    out.append("")
    out.append("#define LOR_MEMORY_ARENA_ALLOC_ARRAY_(...)                                  \\")
    out.append("    LOR_MEMORY_SELECT_ALLOC_ARRAY_(__VA_ARGS__,                             \\")
    out.append("                                   LOR_MEMORY_ARENA_ALLOC_ARRAY_OPTIONS_,   \\")
    out.append("                                   LOR_MEMORY_ARENA_ALLOC_ARRAY_OPTIONS_,   \\")
    out.append("                                   LOR_MEMORY_ARENA_ALLOC_ARRAY_OPTIONS_,   \\")
    out.append("                                   LOR_MEMORY_ARENA_ALLOC_ARRAY_DEFAULT_,   \\")
    out.append("                                   unused)                                  \\")
    out.append("    (__VA_ARGS__)")
    out.append("")
    out.append("/** Allocates an array from an arena with optional designated arguments.")
    out.append(" *")
    out.append(" * Supported forms:")
    out.append(" * `lor_arena_alloc_array(&arena, count, sizeof(*items))`")
    out.append(" * `lor_arena_alloc_array(&arena, count, sizeof(*items), .zero = true)`")
    out.append(" */")
    out.append("#undef lor_arena_alloc_array")
    out.append("#define lor_arena_alloc_array(...) LOR_MEMORY_ARENA_ALLOC_ARRAY_(__VA_ARGS__)")
    out.append("#endif")
    out.append("")
    out.append("/* Leakcheck build mode")
    out.append("   Define LOR_LEAKCHECK for the whole build to route liblor memory")
    out.append("   calls and stdlib heap calls through location-aware tracking. */")
    out.append("#if defined(LOR_LEAKCHECK) && !defined(LOR_MEMORY_NO_LOCATION_MACROS)")
    out.append("#undef lor_arena_init_config")
    out.append("#define lor_arena_init_config(arena, config) \\")
    out.append("    lor_arena_init_config_debug((arena), (config), __FILE__, __LINE__)")
    out.append("#undef lor_mmap_file")
    out.append("#define lor_mmap_file(path, mode) \\")
    out.append("    lor_mmap_file_debug((path), (mode), __FILE__, __LINE__)")
    out.append("#endif")
    out.append("")
    out.append("#if defined(LOR_LEAKCHECK) && !defined(LOR_MEMORY_NO_STDLIB_MACROS)")
    out.append("#define malloc(size) lor_malloc_debug((size), __FILE__, __LINE__)")
    out.append("#define calloc(count, elem_size) \\")
    out.append("    lor_calloc_debug((count), (elem_size), __FILE__, __LINE__)")
    out.append("#define realloc(ptr, size) lor_realloc_debug((ptr), (size), __FILE__, __LINE__)")
    out.append("#define free(ptr) lor_free_debug((ptr), __FILE__, __LINE__)")
    out.append("#define strdup(text) lor_strdup_debug((text), __FILE__, __LINE__)")
    out.append("#endif")
    return "\n".join(out) + "\n"


def generate(manifest_path: Path) -> str:
    manifest = json.loads(read_text(manifest_path))
    modules = manifest["modules"]

    out = []
    out.append("// SPDX-License-Identifier: MIT")
    out.append("/*")
    out.append("   liblor in a single-header.")
    out.append("   Generated by tools/gen_single_header.py; do not edit by hand.")
    out.append("*/")
    out.append("")
    out.append("#ifndef LOR_SINGLE_HEADER_H")
    out.append("#define LOR_SINGLE_HEADER_H")
    out.append("#define LOR_SINGLE_HEADER_BUILD")
    out.append("")
    out.append(emit_module_selection(modules).rstrip())
    out.append("")
    out.append(emit_custom_prefix(modules).rstrip())
    out.append("")
    out.append(emit_declarations(modules).rstrip())
    out.append(emit_implementations(modules).rstrip())
    out.append(emit_strip_prefix_aliases(modules).rstrip())
    out.append("")
    out.append("#undef LOR_SINGLE_HEADER_BUILD")
    out.append(emit_late_macros().rstrip())
    out.append("")
    out.append("#endif /* LOR_SINGLE_HEADER_H */")
    out.append("")
    return "\n".join(out)


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate lor.h.")
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    args = parser.parse_args()

    output = args.output
    if not output.is_absolute():
        output = ROOT / output

    manifest = args.manifest
    if not manifest.is_absolute():
        manifest = ROOT / manifest

    write_text(output, generate(manifest))
    print(f"generated {output.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
