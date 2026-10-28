"""Run generated Dart values and operations against a C implementation."""

import shutil
import sys
import unittest
from pathlib import Path
from tempfile import TemporaryDirectory

from support import (
    FIXTURES,
    PROTOCOLS_STUB,
    ROOT,
    parse,
    parse_sources,
    protocol_header,
    real_api,
    require_tool,
)

from tools.bindgen.compiler import compile_api
from tools.bindgen.emitters import dart, dart_native
from tools.bindgen.schema import validate

BINDING = ROOT / "bindings/dart"


# An event source and a handle that is not one, each closed by a void release.
EVENT_SOURCE_AND_LEASE = """
typedef unsigned long long mln_map BIND("kind=handle;release=mln_map_close");
typedef unsigned long long mln_lease BIND("kind=handle;release=mln_lease_close");
mln_status mln_map_open(
  mln_map* out_map BIND("direction=out"), mln_diagnostic* out_diagnostic
);
mln_status mln_lease_open(
  mln_lease* out_lease BIND("direction=out"), mln_diagnostic* out_diagnostic
);
void mln_map_close(mln_map map);
void mln_lease_close(mln_lease lease);
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


class DartEmitterTests(unittest.TestCase):
    def test_every_owner_has_the_same_handle_members(self):
        generated = dart.lower(parse(EVENT_SOURCE_AND_LEASE)).files[dart.OPERATIONS]
        for owner in ("MapHandle", "LeaseHandle"):
            self.assertIn(
                f"String toString() => '{owner}(id=0x${{id.toRadixString(16)}}, "
                "closed=$isClosed)';",
                generated,
            )
        self.assertEqual(generated.count("bool get isClosed => _state.isClosed;"), 2)
        # Only an event source's id doc names events.
        self.assertEqual(generated.count("which runtime events report"), 1)
        self.assertIn(
            "which runtime events report", generated.split("class MapHandle")[1]
        )
        self.assertIn("  /// The native handle id.\n  int get id", generated)

    def test_a_drain_returns_its_slice_and_releases_its_batch(self):
        api = parse(SLICE_DRAIN, defines=("MLN_PROTOCOL_STANDARD_TYPES",))
        generated = dart.lower(api).files[dart.OPERATIONS]
        # The drain copies the slice, then releases the batch even if the copy
        # throws, and nothing queued reads as an empty list.
        self.assertIn("List<Reading> drainReadings() =>", generated)
        self.assertIn("return const [];", generated)
        self.assertIn(
            "return _drain(outBatch.value, (batch) { raw.mln_reading_batch_release(batch); }, "
            "() => _readReadingView(outView.ref));",
            generated,
        )
        self.assertIn(
            "List<Reading> _readReadingView(raw.mln_reading_view source) =>", generated
        )
        # Neither the batch nor its view is public.
        self.assertNotIn("ReadingBatchHandle", generated)
        self.assertNotIn("class ReadingView", dart.lower(api).files[dart.VALUES])

    def test_a_callback_receives_its_slice_argument_as_a_list(self):
        values = dart.lower(real_api()).files[dart.VALUES]
        self.assertIn(
            "typedef RuntimeEventCallback = void Function(List<RuntimeEvent> events);",
            values,
        )
        self.assertNotIn("EventBatchView", values)
        self.assertIn(
            "callback(_readEventBatchView(events.ref));",
            dart.lower(real_api()).files[dart.OPERATIONS],
        )

    def test_a_queued_callback_runs_through_the_pointer_of_its_type(self):
        operations = dart.lower(real_api()).files[dart.OPERATIONS]
        # A registration names the isolate's queue beside the pointer, and its
        # release retires the token.
        self.assertIn(
            "result.ref.callback = _deliverResourceProviderCallback;", operations
        )
        self.assertIn("result.ref.release_user_data = dispatchedRelease;", operations)
        # A wake without a native registration runs through the queue too.
        self.assertIn(
            "if (native == null) return _prepareQueuedWake(value, roots);", operations
        )
        # A call whose owner is gone releases the handle it owns, which fails
        # the request.
        self.assertRegex(
            operations,
            r"_NativeResourceRequest\(handle\)\);\s*if \(callback == null\) \{\s*owner\.close\(\);",
        )

    def test_a_registration_of_one_callback_is_the_callback(self):
        values = dart.lower(real_api()).files[dart.VALUES]
        operations = dart.lower(real_api()).files[dart.OPERATIONS]
        self.assertNotIn("class RuntimeEventHandler", values)
        self.assertIn("setEventHandler(RuntimeEventCallback handler)", operations)
        # An optional lone callback makes its record member optional.
        self.assertIn("final CameraTransitionEndCallback? endHandler;", values)
        # A registration with more than its callback stays a class.
        self.assertIn("final class ResourceProvider {", values)

    def test_a_required_member_has_no_default_and_others_default_to_zero(self):
        values = dart.lower(real_api()).files[dart.VALUES]
        self.assertIn(
            "const MapOptions({required this.initialExtent, this.mapMode = const "
            "MapMode.fromRawValue(0), this.fastPforEnabled = false, this.eventMask});",
            values,
        )
        # A list member defaults to empty unless it is required.
        self.assertIn("List<String> tileUrls = const []", values)
        self.assertIn("required List<MetalBorrowedTexture> textures", values)
        # Reading the C version skips the ABI version check, which reads it.
        operations = dart.lower(real_api()).files[dart.OPERATIONS]
        self.assertIn(
            "int cVersion() {\n    final nativeResult = raw.mln_c_version();",
            operations,
        )

    def test_a_root_constructor_is_a_factory_and_a_global_names_its_verb_first(
        self,
    ):
        operations = dart.lower(real_api()).files[dart.OPERATIONS]
        runtime = operations.split("final class RuntimeHandle ")[1].split("\n}\n")[0]
        self.assertIn(
            "  static RuntimeHandle create(RuntimeOptions options) => "
            "withNativeArena((arena) {\n    ensureAbiVersion();",
            runtime,
        )
        self.assertIn(
            "\n  void clearLogCallback() {\n    ensureAbiVersion();", operations
        )

    def test_docs_name_the_dart_symbols_of_c_declarations(self):
        values = dart.lower(real_api()).files[dart.VALUES]
        self.assertIn(
            "  /// `MapHandle.cancelTransitions()`, `MapHandle.cancelCameraTransition()`, or\n"
            "  /// `GesturePhase.cancel` ended the transitions",
            values,
        )
        self.assertNotIn("`mln_map_cancel_transitions()`", values)

    def test_a_record_of_only_a_tag_and_its_union_is_the_union(self):
        values = dart.lower(real_api()).files[dart.VALUES]
        self.assertIn("sealed class RenderedQueryGeometry {", values)
        self.assertNotIn("RenderedQueryGeometryData", values)
        # Native never returns a query geometry, so it has no unknown variant;
        # it returns a region definition, which keeps one.
        self.assertNotIn("RenderedQueryGeometryUnknown", values)
        self.assertIn(
            "final class OfflineRegionDefinitionUnknown extends OfflineRegionDefinition {",
            values,
        )

    def test_generated_values_and_keyword_parameters_round_trip(self):
        dart_tool = require_tool(self, "dart", BINDING)
        clang = require_tool(self, "clang")
        header = protocol_header(
            groups=(
                "caller_sized_output",
                "caller_sized_text",
                "values",
                "keywords",
                "defaults",
                "absent_value",
            )
        )
        api = parse_sources({"api.h": header})
        validate(api)
        self.assertEqual(
            set(dart.lower(api).generated),
            {
                "mln_keyword_combine",
                "mln_probe_batch_points",
                "mln_probe_batch_scores",
                "mln_probe_zeros_json",
                "mln_probe_read_level",
                "mln_probe_roundtrip",
                "mln_probe_settings_check",
            },
        )
        with TemporaryDirectory() as directory:
            root = Path(directory)
            library = root / (
                "libprobe.dylib" if sys.platform == "darwin" else "libprobe.so"
            )
            clang.run(
                self,
                "-std=c2x",
                "-shared",
                "-fPIC",
                f"-I{FIXTURES}",
                str(PROTOCOLS_STUB),
                "-o",
                str(library),
                cwd=root,
            )
            (root / "render").mkdir()
            shutil.copy(BINDING / "lib/src/render/native_pointer.dart", root / "render")
            shutil.copy(BINDING / "lib/src/values.dart", root)
            (root / "internal/callback").mkdir(parents=True)
            shutil.copy(
                BINDING / "lib/src/internal/callback/native_registration.dart",
                root / "internal/callback",
            )
            # The probe groups name no handle owners, so an empty library
            # stands in for the runtime that the binding's values import.
            (root / "runtime").mkdir()
            (root / "runtime/runtime.dart").write_text("export '../runtime.dart';\n")
            (root / "generated_values.dart").write_text(
                dart.lower(api).files[dart.VALUES]
            )
            (root / "generated_operations.dart").write_text(
                dart.lower(api).files[dart.OPERATIONS]
            )
            (root / "maplibre_native_c.g.dart").write_text(
                dart_native.generate(compile_api(api))
            )
            shutil.copy(BINDING / "lib/src/internal/c/native_asset.dart", root)
            for path in (FIXTURES / "probes/dart").glob("*.dart"):
                shutil.copy(path, root)
            dart_tool.run(
                self,
                "run",
                str(root / "main.dart"),
                cwd=root,
                env={"MLN_PROBE_LIBRARY": str(library)},
                timeout=300,
            )


if __name__ == "__main__":
    unittest.main()
