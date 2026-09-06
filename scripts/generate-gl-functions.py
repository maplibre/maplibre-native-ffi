#!/usr/bin/env python3
"""Rewrites MapLibre Native's GL entry-point table to resolve at run time.

Upstream defines `mln::platform::glFoo` as a function pointer initialized from
the linked `::glFoo`, which is what makes the library depend on a GL loader at
link time. Every initializer here becomes a stub that looks the entry point up
through the client library on its first call, so the table costs nothing at link
time and the shipped binaries carry no GL dependency from the build host. The
lookup waits for that first call because a shared library runs its initializers
as it loads, which can precede the host loading its client library.

Generating this from the upstream file rather than checking a copy in keeps the
two from drifting when upstream adds or removes an entry point.
"""

from __future__ import annotations

import argparse
import pathlib
import re
import sys

# `= ::glFoo;`, which upstream wraps onto its own line for long declarations.
INITIALIZER = re.compile(r"=\s*::(gl\w+);")

PROLOGUE = """
// Generated from %s by scripts/generate-gl-functions.py. Edit neither; change
// the generator.

#include "render/opengl/gl_resolve.hpp"

// Each stub takes the name it resolves as a template argument, which needs an
// object rather than a literal.
namespace {
%s}  // namespace
"""

# The prefix keeps these clear of the `mln::platform` names they stand for.
NAME_PREFIX = "mln_gl_name_"


def rewrite(source: str, origin: str) -> tuple[str, int]:
    def replace(match: re.Match[str]) -> str:
        name = match.group(1)
        return (
            f"= &::mln::core::opengl::gl_entry_point<\n"
            f"    std::remove_const_t<decltype({name})>, {NAME_PREFIX}{name}>::call;"
        )

    names = [match.group(1) for match in INITIALIZER.finditer(source)]
    rewritten, count = INITIALIZER.subn(replace, source)
    if count == 0:
        raise SystemExit(f"error: no GL entry points found in {origin}")

    # The prototypes only existed to give the initializers something to bind to.
    rewritten = rewritten.replace("#define GL_GLEXT_PROTOTYPES\n", "")
    marker = "#include <mln/platform/gl_functions.hpp>\n"
    if marker not in rewritten:
        raise SystemExit(f"error: unexpected header layout in {origin}")
    declarations = "".join(
        f'constexpr char {NAME_PREFIX}{name}[] = "{name}";\n' for name in names
    )
    prologue = PROLOGUE % (origin, declarations)
    return rewritten.replace(marker, marker + prologue, 1), count


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=pathlib.Path)
    parser.add_argument("output", type=pathlib.Path)
    arguments = parser.parse_args()

    rewritten, count = rewrite(arguments.source.read_text(), arguments.source.name)
    arguments.output.parent.mkdir(parents=True, exist_ok=True)
    arguments.output.write_text(rewritten)
    print(f"rewrote {count} GL entry points into {arguments.output}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
