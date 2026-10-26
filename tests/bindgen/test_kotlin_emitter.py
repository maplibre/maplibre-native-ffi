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

from tools.bindgen.emitters import kotlin, kotlin_callbacks
from tools.bindgen.emitters.kotlin_abi import Abi, LayoutError

GENERATED = "src/commonMain/kotlin/org/maplibre/nativeffi"

# A handle whose dispose is also its release and takes an optional completion,
# as the render session's does.
DISPOSE_WITH_COMPLETION = """
typedef unsigned long long mln_lease BIND(
  "kind=handle;release=mln_lease_dispose;abandon=mln_lease_abandon;"
  "dispose=mln_lease_dispose"
);
mln_status mln_lease_open(
  mln_lease* out_lease BIND("direction=out"), mln_diagnostic* out_diagnostic
);
mln_status mln_lease_abandon(mln_lease lease, mln_diagnostic* out_diagnostic);
BIND("execution=lifecycle")
mln_status mln_lease_dispose(
  mln_lease lease, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
);
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
  mln_reading_batch* out_batch BIND("direction=out"),
  mln_reading_view* out_view BIND("direction=out"),
  mln_diagnostic* out_diagnostic
);
"""

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

    def test_the_wasm_glue_compiles_against_the_headers_for_wasm32(self):
        # Each wrapper casts its wasm32 carriers to the C parameter types, and a
        # thread-start wrapper asserts that its arguments fit the thread start,
        # so compiling the glue against the real prototypes checks both.
        clang = require_tool(self, "clang").executable
        source = kotlin.generate(real_api())[kotlin.WASM_GLUE]
        with TemporaryDirectory() as directory:
            check = Path(directory) / "glue.c"
            check.write_text(source)
            run(
                self,
                [
                    clang,
                    "--target=wasm32-unknown-unknown",
                    "-ffreestanding",
                    "-std=c2x",
                    "-fsyntax-only",
                    "-Wall",
                    "-Werror",
                    "-DMLN_STATIC",
                    f"-I{ROOT / 'include'}",
                    f"-I{NATIVE_INCLUDE}",
                    f"-I{ROOT / 'bindings/kotlin/wasm'}",
                    str(check),
                ],
                ROOT,
            )

    def test_a_frame_has_no_scoped_cleanup(self):
        owners = kotlin.generate(real_api())
        # A frame's release needs the host's consumer sync and its discard
        # quarantines a slot, so a frame has no `use`, and its release states
        # the consumer's completion.
        frame = owners[f"{GENERATED}/AcquiredFrameHandle.kt"]
        self.assertNotIn("AutoCloseable", frame)
        self.assertIn("public fun release(consumerCompletion: GpuSync?): Unit =", frame)

    def test_every_owner_has_the_same_handle_members(self):
        owners = kotlin.generate(real_api())
        for handle in (
            "Runtime",
            "Map",
            "RenderSession",
            "AcquiredFrame",
            "MapProjection",
        ):
            owner = owners[f"{GENERATED}/{handle}Handle.kt"]
            self.assertIn("public val id: ULong", owner)
            self.assertIn("val isClosed: Boolean get() = binding.isReleased", owner)
            self.assertIn(
                f'"{handle}Handle(id=0x${{binding.handle.toString(16)}}, closed=$isClosed)"',
                owner,
            )
            # Only the handles that runtime events name are their source.
            self.assertEqual(
                handle in {"Runtime", "Map"},
                "runtime events report as their source" in owner,
                handle,
            )
        # A handle with a dispose closes by disposing, and its suspending
        # `use` awaits the release.
        for handle in ("Runtime", "Map"):
            owner = owners[f"{GENERATED}/{handle}Handle.kt"]
            self.assertIn(") : AsyncReleasable, AutoCloseable {", owner)
            self.assertIn("public override fun close() { dispose() }", owner)
        # A release that completes at once is the handle's only `close()`.
        projection = owners[f"{GENERATED}/MapProjectionHandle.kt"]
        self.assertIn(
            "public override fun close(): Unit = nativeClose(this, binding) "
            "{ C.mln_map_projection_release(handle) }",
            projection,
        )
        self.assertNotIn("fun release(", projection)

    def test_a_dispose_with_a_completion_is_the_release_and_the_dispose(self):
        outputs = kotlin.generate(parse(DISPOSE_WITH_COMPLETION))
        owner = outputs[f"{GENERATED}/LeaseHandle.kt"]
        # The release reports retirement through the completion, and the
        # dispose, like every disposer, passes none.
        self.assertIn(
            "public override fun release(): Deferred<Unit> = nativeRetire(this, binding) "
            "{ check(C.mln_lease_dispose(handle, completion, diagnostic)) }",
            owner,
        )
        self.assertIn(
            "public fun dispose(): Unit = nativeClose(this, binding) "
            "{ check(C.mln_lease_dispose(handle, 0L, diagnostic)) }",
            owner,
        )
        self.assertIn(
            "C.mln_lease_dispose(handle, 0L, diagnostic)",
            outputs[f"{GENERATED}/GeneratedOwnerDisposal.kt"],
        )
        # The suspending `use` awaits the release, and `use` disposes.
        self.assertIn(") : AsyncReleasable, AutoCloseable {", owner)
        self.assertIn("public override val isClosed: Boolean", owner)
        self.assertIn("public override fun close() { dispose() }", owner)

    def test_a_drain_returns_its_slice_and_releases_its_batch(self):
        outputs = kotlin.generate(
            parse(SLICE_DRAIN, defines=("MLN_PROTOCOL_STANDARD_TYPES",))
        )
        api = outputs[f"{GENERATED}/GeneratedGlobals.kt"]
        # The drain copies the slice, then releases the batch even if the copy
        # throws, and nothing queued reads as an empty list.
        self.assertIn("public fun drainReadings(): List<Reading> =", api)
        self.assertIn(
            "if (present(C.mln_drain_readings(batch, out, diagnostic), absent = -9)) "
            "drain(batch, { C.mln_reading_batch_release(it) }) "
            "{ readReadingView(out) } else emptyList()",
            api,
        )
        codecs = outputs[f"{GENERATED}/GeneratedCodecs.kt"]
        self.assertIn(
            "internal fun readReadingView(source: Long): List<Reading> = readStrided(",
            codecs,
        )
        # Neither the batch nor its view is public.
        values = outputs[f"{GENERATED}/GeneratedValues.kt"]
        self.assertNotIn("class ReadingView", values)
        self.assertFalse(any("ReadingBatch" in path for path in outputs))

    def test_a_callback_receives_its_slice_argument_as_a_list(self):
        outputs = kotlin.generate(real_api())
        values = outputs[f"{GENERATED}/GeneratedValues.kt"]
        self.assertIn("public operator fun invoke(events: List<RuntimeEvent>)", values)
        self.assertNotIn("EventBatchView", values)
        self.assertIn(
            "value.callback(readEventBatchView(events))",
            outputs[
                "src/commonMain/kotlin/org/maplibre/nativeffi/internal/c/Upcalls.kt"
            ],
        )

    def test_a_required_member_has_no_default_and_others_default_to_zero(self):
        outputs = kotlin.generate(real_api())
        values = outputs[f"{GENERATED}/GeneratedValues.kt"]
        options = values.split("public data class MapOptions(")[1].split("\n)")[0]
        self.assertIn("public val initialExtent: LogicalExtent,", options)
        self.assertIn("public val fastPforEnabled: Boolean = false", options)
        self.assertIn("public val eventMask: RuntimeEventFlag? = null", options)
        self.assertIn("public val extent: LogicalExtent = LogicalExtent()", values)
        self.assertNotIn("companion object { internal val default", values)
        # The ABI version check reads the C version, so reading it skips it.
        self.assertIn(
            "fun cVersion(): UInt = nativeUncheckedCall {",
            outputs[f"{GENERATED}/GeneratedGlobals.kt"],
        )

    def test_a_root_constructor_is_a_factory_and_a_global_names_its_verb_first(
        self,
    ):
        outputs = kotlin.generate(real_api())
        runtime = outputs[f"{GENERATED}/RuntimeHandle.kt"]
        self.assertIn("  public companion object {\n", runtime)
        self.assertIn(
            "public suspend fun create(options: RuntimeOptions): RuntimeHandle =",
            runtime.split("public companion object {")[1],
        )
        api = outputs[f"{GENERATED}/GeneratedGlobals.kt"]
        # Globals are top-level functions of the package.
        self.assertNotIn("object ", api)
        self.assertNotIn("Create(", api)
        self.assertIn("\npublic fun clearLogCallback(): Unit =", api)

    def test_a_record_of_only_a_tag_and_its_union_is_the_union(self):
        values = kotlin.generate(real_api())[f"{GENERATED}/GeneratedValues.kt"]
        self.assertIn("public sealed interface RenderedQueryGeometry {", values)
        self.assertNotIn("RenderedQueryGeometryData", values)
        # Native never returns a query geometry, so it has no unknown variant;
        # it returns a region definition, which keeps one.
        geometry = values.split("public sealed interface RenderedQueryGeometry {")[1]
        self.assertNotIn("Unknown", geometry.split("\n}")[0])
        definition = values.split("public sealed interface OfflineRegionDefinition {")[
            1
        ]
        self.assertIn("public data class Unknown(", definition.split("\n}")[0])

    def test_docs_name_the_binding_symbols_of_c_declarations(self):
        values = kotlin.generate(real_api())[f"{GENERATED}/GeneratedValues.kt"]
        self.assertIn(
            "`MapHandle.cancelTransitions()`, `MapHandle.cancelCameraTransition()`, or\n"
            "     * `GesturePhase.CANCEL` ended the transitions",
            values,
        )
        self.assertNotIn("`mln_map_cancel_transitions()`", values)

    def test_split_registrations_take_a_form_per_target(self):
        _functions, _unsupported, bodies, split, values, _native = kotlin.lowered(
            real_api()
        )
        for lowering in split:
            values.split_codecs(lowering)
        values.codecs()
        # The registrations whose answer a callback computes, or a rule
        # replaces, differ by target, so common code takes none of them.
        self.assertEqual(
            {
                value.native
                for value in values.bound.public_values.values()
                if kotlin_callbacks.splits(value, values)
            },
            {"mln_log_handler", "mln_http_header_transform"},
        )
        registrations = {
            "mln_log_set_callback",
            "mln_runtime_set_http_header_transform",
        }
        response = "mln_http_header_transform_response_set_header"
        self.assertEqual(set(split[kotlin_callbacks.ASYNC]), registrations)
        self.assertEqual(
            set(split[kotlin_callbacks.THREADED]), registrations | {response}
        )
        self.assertFalse((registrations | {response}) & set(bodies))
        # Only the threaded forms answer native from their callbacks, and every
        # other queue-form site has a delivery that the page drains into it.
        for site in kotlin_callbacks.sites(values).values():
            with self.subTest(site=site.name):
                if site.split == kotlin_callbacks.THREADED:
                    self.assertEqual(site.form, "direct")
                elif site.form == "queue":
                    callback = values.bound.callbacks[site.callback]
                    self.assertIsNotNone(callback.delivery)

    def test_callback_results_answer_native_in_their_c_types(self):
        outputs = kotlin.generate(real_api())
        threaded = outputs[
            "src/threadedMain/kotlin/org/maplibre/nativeffi/internal/c/ThreadedUpcalls.kt"
        ]
        # A bool callback answers with a Boolean, and fails with false.
        self.assertIn(
            'upcall<LogHandler, Boolean>("mln_log_callback", userData, false)',
            threaded,
        )
        # A status callback returns nothing and throws to fail, so its site
        # answers success itself and maps a throw to a status.
        self.assertIn(
            'statusUpcall<HttpHeaderTransform>("mln_http_header_transform_callback", '
            "userData, -5)",
            threaded,
        )
        callbacks = outputs[
            "src/threadedMain/kotlin/org/maplibre/nativeffi/GeneratedThreadedCallbacks.kt"
        ]
        self.assertIn("message: String): Boolean\n", callbacks)
        self.assertIn("outResponse: HttpHeaderTransformResponse)\n", callbacks)
        # Kotlin/Wasm's queue form answers native's queue result.
        wasm = outputs[
            "src/wasmJsMain/kotlin/org/maplibre/nativeffi/internal/c/WasmUpcalls.kt"
        ]
        self.assertIn("readViewString(message)); true }; true }", wasm)

    def test_native_registrations_mirror_their_c_records(self):
        _functions, _unsupported, _bodies, _split, values, _native = kotlin.lowered(
            real_api()
        )
        values.codecs()
        # A wake and the queue lock, whose callback native code waits for, take
        # the host's native functions; each parameter names the field it fills.
        self.assertEqual(
            {
                value.native: kotlin_callbacks.native_factory(value, values).parameters
                for value in values.used.values()
                if kotlin_callbacks.accepts_native(value, values)
            },
            {
                "mln_wake": (
                    ("callback", "callback"),
                    ("userData", "user_data"),
                    ("releaseUserData", "release_user_data"),
                ),
                "mln_queue_lock": (
                    ("lock", "lock"),
                    ("unlock", "unlock"),
                    ("userData", "user_data"),
                    ("releaseUserData", "release_user_data"),
                ),
            },
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
