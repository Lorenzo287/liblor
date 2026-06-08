#!/usr/bin/env python3
# SPDX-License-Identifier: MIT

from __future__ import annotations

import argparse
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_MANIFEST = ROOT / "tools" / "lor_modules.json"
DEFAULT_OUTPUT = ROOT / "lor.h"
LATE_MACROS_BEGIN = "// LOR_SINGLE_HEADER_LATE_MACROS_BEGIN"
LATE_MACROS_END = "// LOR_SINGLE_HEADER_LATE_MACROS_END"


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
        # These are only for compiling the normal source file. In the generated
        # header, post-implementation macros are emitted after the source block.
        if stripped in {
            "#define LOR_MEMORY_NO_STDLIB_MACROS",
            "#define LOR_MEMORY_NO_LOCATION_MACROS",
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


def strip_marked_blocks(lines: list[str], begin: str, end: str) -> list[str]:
    result = []
    skipping = False

    for line in lines:
        stripped = line.strip()

        if stripped == begin:
            if skipping:
                raise ValueError(f"nested marker: {begin}")
            skipping = True
            continue

        if skipping:
            if stripped == end:
                skipping = False
            continue

        result.append(line)

    if skipping:
        raise ValueError(f"missing marker: {end}")

    return result


def extract_marked_blocks(path: Path, begin: str, end: str) -> list[str]:
    blocks = []
    current: list[str] | None = None

    for line in read_text(path).splitlines():
        stripped = line.strip()

        if stripped == begin:
            if current is not None:
                raise ValueError(f"{path}: nested marker: {begin}")
            current = []
            continue

        if stripped == end:
            if current is None:
                raise ValueError(f"{path}: unmatched marker: {end}")
            blocks.append("\n".join(current).strip() + "\n")
            current = None
            continue

        if current is not None:
            current.append(line)

    if current is not None:
        raise ValueError(f"{path}: missing marker: {end}")

    return blocks


def clean_header(path: Path) -> str:
    guard = path.stem.upper().replace("-", "_") + "_H"
    if path.stem == "lor":
        guard = "LOR_H"
    else:
        guard = f"LOR_{path.stem.upper()}_H"

    lines = read_text(path).splitlines()
    lines = strip_spdx(lines)
    lines = strip_local_includes(lines)
    lines = strip_marked_blocks(lines, LATE_MACROS_BEGIN, LATE_MACROS_END)
    lines = strip_header_guard(lines, guard)
    return "\n".join(lines).strip() + "\n"


def clean_source(path: Path) -> str:
    lines = read_text(path).splitlines()
    lines = strip_spdx(lines)
    lines = strip_local_includes(lines)
    return "\n".join(lines).strip() + "\n"


def module_enabled_condition(module: dict) -> str:
    return module["enable_macro"]


def module_dependency_macros(module: dict, modules: list[dict]) -> list[str]:
    modules_by_name = {item["name"]: item for item in modules}
    result = []
    visiting = set()
    visited = set()

    def visit(name: str) -> None:
        if name in visited:
            return
        if name in visiting:
            raise ValueError(f"cyclic module dependency involving {name}")
        if name not in modules_by_name:
            raise ValueError(f"unknown module dependency: {name}")

        visiting.add(name)
        dependency = modules_by_name[name]
        for nested in dependency.get("dependencies", []):
            visit(nested)
        visiting.remove(name)
        visited.add(name)
        result.append(dependency["enable_macro"])

    for dependency in module.get("dependencies", []):
        visit(dependency)

    return result


def order_modules(modules: list[dict]) -> list[dict]:
    modules_by_name = {module["name"]: module for module in modules}
    result = []
    visiting = set()
    visited = set()

    def visit(module: dict) -> None:
        name = module["name"]
        if name in visited:
            return
        if name in visiting:
            raise ValueError(f"cyclic module dependency involving {name}")

        visiting.add(name)
        for dependency_name in module.get("dependencies", []):
            if dependency_name not in modules_by_name:
                raise ValueError(f"unknown module dependency: {dependency_name}")
            visit(modules_by_name[dependency_name])
        visiting.remove(name)
        visited.add(name)
        result.append(module)

    for module in modules:
        visit(module)
    return result


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

    for module in modules:
        enabled_by = module.get("enabled_by", [])
        if not enabled_by:
            continue
        condition = " || ".join(f"defined({macro})" for macro in enabled_by)
        out.append("")
        out.append(f"#if {condition}")
        out.append(f"#define {module['enable_macro']}")
        out.append("#endif")

    for module in modules:
        dependencies = module_dependency_macros(module, modules)
        if not dependencies:
            continue
        out.append("")
        out.append(f"#ifdef {module['enable_macro']}")
        for macro in dependencies:
            out.append(f"#define {macro}")
        out.append("#endif")

    return "\n".join(out) + "\n"


def emit_declarations(modules: list[dict]) -> str:
    out = []
    for module in modules:
        path = ROOT / module["header"]
        out.append(f"// === {module['name']}: declarations ===")
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
        source = module.get("source")
        if source is None:
            continue
        path = ROOT / source
        out.append(f"// === {module['name']}: implementation ===")
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
    out.append("   symbol names. */")
    out.append("#ifdef LOR_STRIP_PREFIX")
    for module in modules:
        out.append(f"#ifdef {module_enabled_condition(module)}")
        for group in ("types", "macros", "functions"):
            for canonical, alias in module["symbols"].get(group, []):
                out.append(f"#define {alias} {canonical}")
        out.append("#endif")
    out.append("#endif")
    return "\n".join(out) + "\n"


def emit_late_macros(modules: list[dict]) -> str:
    """Emit marked macro blocks after single-header implementations.

    Some public macros intentionally wrap function names, so emitting them with
    declarations would also rewrite the generated function definitions.
    """
    out = []

    for module in modules:
        path = ROOT / module["header"]
        blocks = extract_marked_blocks(path, LATE_MACROS_BEGIN, LATE_MACROS_END)
        for block in blocks:
            out.append(f"// === {module['name']}: post-implementation macros ===")
            out.append(f"#ifdef {module_enabled_condition(module)}")
            out.append(block.rstrip())
            out.append("#endif")
            out.append("")

    return "\n".join(out).rstrip() + "\n" if out else ""


def generate(manifest_path: Path) -> str:
    manifest = json.loads(read_text(manifest_path))
    modules = order_modules(manifest["modules"])

    out = []
    out.append("// SPDX-License-Identifier: MIT")
    out.append("")
    out.append("/* liblor in a single-header.")
    out.append("   Generated by tools/gen_single_header.py; do not edit by hand. */")
    out.append("")
    out.append("#ifndef LOR_SINGLE_HEADER_H")
    out.append("#define LOR_SINGLE_HEADER_H")
    out.append("#define LOR_SINGLE_HEADER_BUILD")
    out.append("")
    out.append(emit_module_selection(modules).rstrip())
    out.append("")
    out.append(emit_declarations(modules).rstrip())
    out.append(emit_implementations(modules).rstrip())
    out.append(emit_strip_prefix_aliases(modules).rstrip())
    out.append("")
    out.append("#undef LOR_SINGLE_HEADER_BUILD")

    late_macros = emit_late_macros(modules).rstrip()
    if late_macros:
        out.append(late_macros)
        out.append("")

    out.append("#endif // LOR_SINGLE_HEADER_H")
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
