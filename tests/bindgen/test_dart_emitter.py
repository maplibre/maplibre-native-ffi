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
        generated = dart.generate(parse(EVENT_SOURCE_AND_LEASE))
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
        generated = dart.generate(api)
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
        self.assertNotIn("class ReadingView", dart.generate_values(api))

    def test_a_callback_receives_its_slice_argument_as_a_list(self):
        values = dart.generate_values(real_api())
        self.assertIn(
            "typedef RuntimeEventCallback = void Function(List<RuntimeEvent>);", values
        )
        self.assertNotIn("EventBatchView", values)
        self.assertIn(
            "callback(_readEventBatchView(payload.events.ref));",
            dart.generate(real_api()),
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
                "default_registration",
                "absent_value",
            )
        )
        api = parse_sources({"api.h": header})
        validate(api)
        self.assertEqual(
            set(dart.coverage(api)["generated"]),
            {
                "mln_keyword_combine",
                "mln_probe_batch_points",
                "mln_probe_batch_scores",
                "mln_probe_zeros_json",
                "mln_probe_hooks_default",
                "mln_probe_read_level",
                "mln_probe_roundtrip",
                "mln_probe_settings_check",
                "mln_probe_settings_default",
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
            (root / "generated_values.dart").write_text(dart.generate_values(api))
            (root / "generated_operations.dart").write_text(dart.generate(api))
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
