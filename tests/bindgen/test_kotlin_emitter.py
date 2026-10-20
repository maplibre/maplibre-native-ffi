"""The Kotlin emitter's layouts and native shims, checked by the C compiler."""

import sys
import unittest
from pathlib import Path
from tempfile import TemporaryDirectory

from support import (
    NATIVE_INCLUDE,
    ROOT,
    parse,
    protocol_groups,
    real_api,
    require_tool,
    run,
)

from tools.bindgen.emitters import kotlin
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
        clang = require_tool(self, "clang").executable
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
                        "maplibre_native_c/binding_support.h",
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

    def test_the_jni_glue_compiles_against_the_headers_for_both_data_models(self):
        # The glue casts each carrier to its C parameter type, so compiling it
        # against the real prototypes checks every cast.
        java = Path(require_tool(self, "java", ROOT / "bindings/kotlin").executable)
        home = java.resolve().parents[1]
        machine = next((home / "include").glob("*/jni_md.h")).parent
        source = kotlin.generate(real_api())[kotlin.JNI]
        for width in TARGETS:
            with self.subTest(width=width):
                self.compile(
                    (),
                    source,
                    width,
                    ROOT / "include",
                    NATIVE_INCLUDE,
                    ROOT / "bindings/kotlin/src/androidMain/jni",
                    home / "include",
                    machine,
                )

    def test_failed_jni_upcall_releases_owned_arguments(self):
        java = Path(require_tool(self, "java", ROOT / "bindings/kotlin").executable)
        home = java.resolve().parents[1]
        machine = next((home / "include").glob("*/jni_md.h")).parent
        with TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "upcall.c"
            source.write_text(kotlin.generate(real_api())[kotlin.JNI] + UPCALL_TEST)
            executable = root / "upcall"
            require_tool(self, "clang").run(
                self,
                "-std=c2x",
                "-DMLN_STATIC",
                "-ffunction-sections",
                "-fdata-sections",
                (
                    "-Wl,-dead_strip"
                    if sys.platform == "darwin"
                    else "-Wl,/OPT:REF"
                    if sys.platform == "win32"
                    else "-Wl,--gc-sections"
                ),
                f"-I{ROOT / 'include'}",
                f"-I{NATIVE_INCLUDE}",
                f"-I{ROOT / 'bindings/kotlin/src/androidMain/jni'}",
                f"-I{home / 'include'}",
                f"-I{machine}",
                str(source),
                "-o",
                str(executable),
                cwd=root,
            )
            run(self, [str(executable)], root)


UPCALL_TEST = r"""
#include <assert.h>
static bool upcall_succeeds;
static unsigned calls;
static mln_resource_transform_request released;

bool mln_jni_upcall_void(mln_jni_upcall* upcall, const jvalue* arguments) {
  (void)upcall;
  assert(arguments[3].j == 41);
  ++calls;
  return upcall_succeeds;
}

void mln_resource_transform_request_release(mln_resource_transform_request request) {
  assert(released == 0);
  released = request;
}

int main(void) {
  mln_jni_resourceTransformCallback(NULL, MLN_RESOURCE_KIND_STYLE, "custom://style.json", 41);
  assert(calls == 1 && released == 41);
  upcall_succeeds = true;
  released = 0;
  mln_jni_resourceTransformCallback(NULL, MLN_RESOURCE_KIND_STYLE, "custom://style.json", 41);
  assert(calls == 2 && released == 0);
}
"""


if __name__ == "__main__":
    unittest.main()
