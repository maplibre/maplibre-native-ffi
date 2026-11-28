"""The Kotlin emitter's layouts and native shims, checked by the C compiler."""

import sys
import unittest
from pathlib import Path
from tempfile import TemporaryDirectory

from support import (
    ROOT,
    emitted,
    parse,
    protocol_groups,
    protocol_header,
    real_api,
    require_tool,
    run,
)

from tools.bindgen.emitters import kotlin
from tools.bindgen.emitters.kotlin_abi import Abi, LayoutError

GENERATED = "src/commonMain/kotlin/org/maplibre/nativeffi"

# A handle whose dispose is also its release and takes an optional completion,
# as the render session's does.
RELEASE_AND_DISPOSE = """
typedef unsigned long long mln_lease BIND(
  "kind=handle;release=mln_lease_release;abandon=mln_lease_abandon;"
  "dispose=mln_lease_dispose"
);
mln_status mln_lease_open(
  mln_lease* out_lease BIND("direction=out"), mln_diagnostic* out_diagnostic
);
mln_status mln_lease_abandon(mln_lease lease, mln_diagnostic* out_diagnostic);
BIND("execution=lifecycle")
mln_status mln_lease_release(
  mln_lease lease, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
);
mln_status mln_lease_dispose(mln_lease lease, mln_diagnostic* out_diagnostic);
"""

# A drain that publishes an owned batch of strided readings with a slice view
# of it, as the render session's frame-result drain does.
SLICE_DRAIN = """
typedef struct mln_reading {
  double value;
} mln_reading;
typedef struct mln_reading_view {
  uint32_t size;
  uint32_t reading_size;
  const mln_reading* readings BIND("length=reading_count;stride=reading_size");
  size_t reading_count;
} mln_reading_view BIND("kind=slice");
typedef unsigned long long mln_reading_batch BIND(
  "kind=handle;release=mln_reading_batch_release;"
  "dispose=mln_reading_batch_release"
);
void mln_reading_batch_release(mln_reading_batch batch);
BIND("execution=event_batch")
mln_status mln_drain_readings(
  mln_reading_batch* out_batch BIND("direction=out;nullable=true"),
  mln_reading_view* out_view BIND("direction=out"),
  mln_diagnostic* out_diagnostic
);
"""

# One Android target per data model the common Kotlin codecs use.
TARGETS = {
    "armv7a-linux-androideabi24": (32, 8),
    "aarch64-linux-android24": (64, 8),
    "i686-linux-android24": (32, 4),
}


def layout_assertions(api, width: int, scalar64_align: int) -> str:
    """Static assertions that the C compiler agrees with every computed layout."""
    abi = Abi(api)
    lines = ["#include <stddef.h>"]
    for record in api.records:
        if not record.complete or record.name.startswith("@"):
            continue
        try:
            layout = abi.record_named(record.name, width, scalar64_align)
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
    def compile(self, headers, source: str, target: str, *includes: Path):
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
                    f"--target={target}",
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

    def test_record_layouts_match_the_compiler_for_all_data_models(self):
        api = real_api()
        for target, (width, scalar64_align) in TARGETS.items():
            with self.subTest(target=target):
                self.compile(
                    (
                        "maplibre_native_c.h",
                        "maplibre_native_c/binding_support.h",
                    ),
                    layout_assertions(api, width, scalar64_align),
                    target,
                    ROOT / "include",
                )

    def test_fixture_layouts_match_the_compiler_for_all_data_models(self):
        groups = protocol_groups()
        api = parse(groups=groups)
        with TemporaryDirectory() as directory:
            header = Path(directory) / "protocols.h"
            header.write_text(protocol_header(groups=groups))
            for target, (width, scalar64_align) in TARGETS.items():
                with self.subTest(target=target):
                    self.compile(
                        (header,), layout_assertions(api, width, scalar64_align), target
                    )

    def test_the_jni_glue_compiles_against_the_headers_for_all_data_models(self):
        # The glue casts each carrier to its C parameter type, so compiling it
        # against the real prototypes checks every cast.
        java = Path(require_tool(self, "java", ROOT / "bindings/kotlin").executable)
        home = java.resolve().parents[1]
        machine = next((home / "include").glob("*/jni_md.h")).parent
        source = emitted(kotlin, real_api())[kotlin.JNI]
        for target in TARGETS:
            with self.subTest(target=target):
                self.compile(
                    (),
                    source,
                    target,
                    ROOT / "include",
                    ROOT / "bindings/kotlin/src/androidMain/jni",
                    home / "include",
                    machine,
                )

    def test_use_releasing_awaits_the_release_and_use_disposes(self):
        outputs = emitted(kotlin, parse(RELEASE_AND_DISPOSE))
        owner = outputs[f"{GENERATED}/LeaseHandle.kt"]
        self.assertIn(
            "public override fun release(): Completion<Unit> = nativeRetire(this, binding) "
            "{ check(C.mln_lease_release(handle, completion, diagnostic)) }",
            owner,
        )
        self.assertIn(
            "public fun dispose(): Unit = nativeClose(this, binding) "
            "{ check(C.mln_lease_dispose(handle, diagnostic)) }",
            owner,
        )
        self.assertIn(") : AsyncReleasable, AutoCloseable {", owner)
        self.assertIn("public override val isClosed: Boolean", owner)
        self.assertIn("public override fun close() { dispose() }", owner)

    def test_a_drain_returns_its_slice_and_releases_its_batch(self):
        outputs = emitted(
            kotlin, parse(SLICE_DRAIN, defines=("MLN_PROTOCOL_STANDARD_TYPES",))
        )
        api = outputs[f"{GENERATED}/GeneratedGlobals.kt"]
        # The drain copies the slice, then releases the batch even if the copy
        # throws, and nothing queued reads as an empty list.
        self.assertIn("public fun drainReadings(): List<Reading> =", api)
        self.assertIn(
            "if (published(C.mln_drain_readings(batch, out, diagnostic), batch)) "
            "drain(batch, { C.mln_reading_batch_release(it) }) "
            "{ readReadingView(out) } else emptyList()",
            api,
        )
        codecs = outputs[f"{GENERATED}/GeneratedCodecs.kt"]
        self.assertIn(
            "internal fun readReadingView(source: Long): List<Reading> = readStrided(",
            codecs,
        )
        values = outputs[f"{GENERATED}/GeneratedValues.kt"]
        self.assertNotIn("class ReadingView", values)
        self.assertFalse(any("ReadingBatch" in path for path in outputs))

    def test_failed_jni_upcall_releases_owned_arguments(self):
        java = Path(require_tool(self, "java", ROOT / "bindings/kotlin").executable)
        home = java.resolve().parents[1]
        machine = next((home / "include").glob("*/jni_md.h")).parent
        with TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "upcall.c"
            source.write_text(emitted(kotlin, real_api())[kotlin.JNI] + UPCALL_TEST)
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
static const char url[] = "custom://style.json";

bool mln_jni_upcall_void(mln_jni_upcall* upcall, const jvalue* arguments) {
  (void)upcall;
  // A by-value view reaches Kotlin as its address.
  const mln_buffer_view* view = (const mln_buffer_view*)(intptr_t)arguments[2].j;
  assert(view->data == url && view->size == sizeof url - 1);
  assert(arguments[3].j == 41);
  ++calls;
  return upcall_succeeds;
}

void mln_resource_transform_request_release(mln_resource_transform_request request) {
  assert(released == 0);
  released = request;
}

int main(void) {
  const mln_buffer_view view = {url, sizeof url - 1};
  mln_jni_resourceTransformCallback(NULL, MLN_RESOURCE_KIND_STYLE, view, 41);
  assert(calls == 1 && released == 41);
  upcall_succeeds = true;
  released = 0;
  mln_jni_resourceTransformCallback(NULL, MLN_RESOURCE_KIND_STYLE, view, 41);
  assert(calls == 2 && released == 0);
}
"""


if __name__ == "__main__":
    unittest.main()
