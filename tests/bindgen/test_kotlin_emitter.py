"""The Kotlin emitter's record layouts, checked by the C compiler."""

import shutil
import unittest
from pathlib import Path
from tempfile import TemporaryDirectory

from support import (
    NATIVE_INCLUDE,
    ROOT,
    parse,
    protocol_groups,
    real_api,
    run,
)

from tools.bindgen.emitters.kotlin_abi import Abi, LayoutError

# One target per data model a Kotlin target uses: Android ARM32 is ILP32, and
# every other target is LP64.
TARGETS = {32: "armv7a-linux-androideabi24", 64: "aarch64-linux-android24"}


def layout_assertions(api, width: int) -> str:
    """Static assertions that the C compiler agrees with every computed layout."""
    abi = Abi(api)
    lines = ["#include <stddef.h>"]
    for record in api.records:
        if not record.complete or record.name.startswith("@"):
            continue
        try:
            layout = abi.record_named(record.name, width)
        except LayoutError:
            continue
        lines.append(
            f'_Static_assert(sizeof({record.name}) == {layout.size}, "{record.name}");'
        )
        lines.append(
            f'_Static_assert(_Alignof({record.name}) == {layout.align}, "{record.name}");'
        )
        lines.extend(
            f'_Static_assert(offsetof({record.name}, {name}) == {offset}, "{record.name}.{name}");'
            for name, offset in layout.offsets.items()
        )
    return "\n".join(lines) + "\n"


class KotlinLayoutTests(unittest.TestCase):
    def compile(self, headers, source: str, width: int, *includes: Path):
        clang = shutil.which("clang")
        if clang is None:
            self.skipTest("clang is not installed")
        with TemporaryDirectory() as directory:
            check = Path(directory) / "check.c"
            check.write_text(
                "".join(f'#include "{header}"\n' for header in headers) + source
            )
            # A freestanding target has no C library. jni.h includes stdio.h
            # and uses nothing from it.
            (Path(directory) / "stdio.h").write_text("")
            run(
                self,
                [
                    clang,
                    f"--target={TARGETS[width]}",
                    "-ffreestanding",
                    "-std=c2x",
                    "-fsyntax-only",
                    "-Wall",
                    "-Werror",
                    f"-I{directory}",
                    *(f"-I{path}" for path in includes),
                    str(check),
                ],
                ROOT,
            )

    def test_record_layouts_match_the_compiler_for_both_data_models(self):
        api = real_api()
        for width in TARGETS:
            with self.subTest(width=width):
                self.compile(
                    (
                        "maplibre_native_c.h",
                        "maplibre_native_c/plugin.h",
                        "maplibre_native_c/callback_adapter.h",
                    ),
                    layout_assertions(api, width),
                    width,
                    ROOT / "include",
                    NATIVE_INCLUDE,
                )

    def test_fixture_layouts_match_the_compiler_for_both_data_models(self):
        groups = protocol_groups()
        api = parse(groups=groups)
        with TemporaryDirectory() as directory:
            header = Path(directory) / "protocols.h"
            header.write_text(
                "\n".join(f"#define MLN_PROTOCOL_{g.upper()}" for g in groups)
                + "\n"
                + (ROOT / "tests/bindgen/fixtures/protocols.h").read_text()
            )
            for width in TARGETS:
                with self.subTest(width=width):
                    self.compile((header,), layout_assertions(api, width), width)


if __name__ == "__main__":
    unittest.main()
