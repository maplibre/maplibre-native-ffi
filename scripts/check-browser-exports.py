#!/usr/bin/env python3
"""Checks the generated Kotlin externals against the linked browser module.

The Kotlin/Wasm binding calls the module through JavaScript, so nothing at
either compile step compares the two: Kotlin type-checks a call against a
declaration it was handed, and the module exports whatever it was linked with. A
declaration that survived a header change reaches native as a plausible wrong
call — a pointer where a handle belongs, or one argument short of the hidden
out-pointer a struct return needs.

This reads the export signatures out of the module emcc wrote and compares each
one against the declaration the binding compiles against. Both readings describe
the shipped artifact, so a mismatch is a real defect rather than a modelling
disagreement.

The module keeps its export names because it is linked with `-lexports.js`; an
optimized link otherwise renames them to one- and two-letter names.
"""

from __future__ import annotations

import argparse
import os
import pathlib
import re
import sys

REPO_ROOT = pathlib.Path(__file__).resolve().parent.parent

DEFAULT_EXTERNS = (
    REPO_ROOT
    / "bindings/kotlin/src/wasmJsMain/generated/org/maplibre/nativeffi"
    / "internal/wasm/generated/EntryPoints.kt"
)
DEFAULT_MODULE = (
    REPO_ROOT
    / "build/emscripten-wasm32-webgl/install/lib/browser/maplibre_native_c.wasm"
)

#: The Emscripten allocator the binding's heap arena calls directly. It is
#: hand-written rather than generated, so its presence is asserted here.
REQUIRED_EXPORTS = ("malloc", "free")

#: How a Kotlin type crosses into wasm. `Long` is 64-bit and reaches the module
#: as a BigInt; everything else the generator emits is 32-bit or floating point.
WASM_TYPES = {"Int": "i32", "Long": "i64", "Float": "f32", "Double": "f64"}

DECLARATION = re.compile(r"internal external fun (mln_\w+)\(([^)]*)\)(?:\s*:\s*(\w+))?")
PARAMETER = re.compile(r":\s*(\w+)")


def declared_signatures(externs: pathlib.Path) -> dict[str, tuple[list, list]]:
    text = externs.read_text()
    signatures = {}
    for name, parameters, returned in DECLARATION.findall(text):
        signatures[name] = (
            [WASM_TYPES[kotlin] for kotlin in PARAMETER.findall(parameters)],
            [WASM_TYPES[returned]] if returned else [],
        )
    if not signatures:
        raise SystemExit(f"{externs} declares no entry points")
    return signatures


def exported_signatures(module_path: pathlib.Path) -> dict[str, tuple[list, list]]:
    """Reads every exported function's signature from the linked module.

    The parser is emsdk's own, so it reads what the pinned toolchain wrote.
    """
    emsdk = os.environ.get("EMSDK")
    if not emsdk:
        raise SystemExit(
            "EMSDK is unset. Run this under `mise exec`, which puts the pinned "
            "emsdk in the environment."
        )
    sys.path.insert(0, str(pathlib.Path(emsdk) / "upstream" / "emscripten"))
    from tools import webassembly
    from tools.webassembly import ExternType, SecType, Type

    names = {Type.I32: "i32", Type.I64: "i64", Type.F32: "f32", Type.F64: "f64"}
    with webassembly.Module(str(module_path)) as module:
        types = module.get_types()
        imported = sum(
            1 for entry in module.get_imports() if entry.kind == ExternType.FUNC
        )
        # get_functions() reports code bodies rather than types, so the function
        # section is read directly.
        section = module.get_section(SecType.FUNCTION)
        if section is None:
            raise SystemExit(f"{module_path} has no function section")
        module.seek(section.offset)
        function_types = [module.read_uleb() for _ in range(module.read_uleb())]

        exports = {}
        for export in module.get_exports():
            if export.kind != ExternType.FUNC:
                continue
            defined = export.index - imported
            if not 0 <= defined < len(function_types):
                raise SystemExit(
                    f"{export.name} resolves outside the defined functions; "
                    "the module exports an import, which this API never does"
                )
            signature = types[function_types[defined]]
            exports[export.name] = (
                [names[Type(parameter)] for parameter in signature.params],
                [names[Type(result)] for result in signature.returns],
            )
    return exports


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("module", type=pathlib.Path, nargs="?", default=DEFAULT_MODULE)
    parser.add_argument("--externs", type=pathlib.Path, default=DEFAULT_EXTERNS)
    arguments = parser.parse_args(argv)

    if not arguments.module.exists():
        raise SystemExit(
            f"{arguments.module} does not exist; build the browser module first"
        )
    declared = declared_signatures(arguments.externs)
    exported = exported_signatures(arguments.module)

    if not any(name.startswith("mln_") for name in exported):
        raise SystemExit(
            f"{arguments.module} exports no mln_* name. It was linked without "
            "-lexports.js, so its export names are minified and nothing here "
            "can be attributed to an entry point."
        )

    failures = []
    for name in REQUIRED_EXPORTS:
        if name not in exported:
            failures.append(f"{name} is not exported; the binding's heap calls it")
    for name, signature in sorted(declared.items()):
        if name not in exported:
            failures.append(f"{name} is declared but the module does not export it")
        elif exported[name] != signature:
            failures.append(
                f"{name} is declared {_render(signature)} and exported "
                f"{_render(exported[name])}"
            )
    if failures:
        print(
            f"{arguments.externs.name} disagrees with {arguments.module.name}:",
            file=sys.stderr,
        )
        for failure in failures:
            print(f"  {failure}", file=sys.stderr)
        print(
            "Regenerate with scripts/generate-wasm-externs.py, and rebuild the "
            "browser module if the headers moved.",
            file=sys.stderr,
        )
        return 1
    print(f"{len(declared)} entry points match {arguments.module.name}")
    return 0


def _render(signature: tuple[list, list]) -> str:
    parameters, results = signature
    return f"({', '.join(parameters)}) -> {', '.join(results) or '()'}"


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
