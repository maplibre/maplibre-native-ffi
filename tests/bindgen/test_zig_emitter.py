"""Test generated Zig with the handwritten runtime against a C implementation."""

import shutil
import unittest
from pathlib import Path
from tempfile import TemporaryDirectory

from support import (
    FIXTURES,
    ROOT,
    parse,
    parse_sources,
    protocol_header,
    real_api,
    require_tool,
)

from tools.bindgen.emitters import zig
from tools.bindgen.schema import validate

BINDING = ROOT / "bindings/zig"


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


class ZigEmitterTests(unittest.TestCase):
    def test_a_drain_returns_its_slice_and_releases_its_batch(self):
        api = parse(SLICE_DRAIN, defines=("MLN_PROTOCOL_STANDARD_TYPES",))
        validate(api)
        source = zig.generate(api)["maplibre_native_ffi.zig"]
        # call.drain copies the view's strided readings and releases the batch.
        self.assertIn(
            "pub fn drainReadings(allocator: std.mem.Allocator, diagnostic: "
            "?*diagnostics.Diagnostic) status.Error!OwnedValue([]const Reading) {\n"
            '    return call.drain("mln_drain_readings", .none, {}, Reading, '
            '"readings", "reading_count", "reading_size", '
            "c.mln_reading_batch_release, allocator, diagnostic, "
            "c.MLN_STATUS_NOT_READY);",
            source,
        )
        # Neither the batch nor its view is public.
        self.assertNotIn("pub const ReadingBatch ", source)
        self.assertNotIn("ReadingView", source)

    def test_a_callback_receives_its_slice_argument_as_a_slice(self):
        source = zig.generate(real_api())["maplibre_native_ffi.zig"]
        self.assertIn(
            "*const fn (context: ?*anyopaque, events: []const RuntimeEvent) "
            "anyerror!void,",
            source,
        )
        self.assertNotIn("EventBatchView", source)

    def test_a_callback_names_its_arguments_and_returns_a_bool_result(self):
        source = zig.generate(real_api())["maplibre_native_ffi.zig"]
        self.assertIn(
            '@"callback": *const fn (context: ?*anyopaque, severity: LogSeverity, '
            "event: LogEvent, code: i64, message: []const u8) anyerror!bool,",
            source,
        )
        # The log callback's failure is false, which an error answers.
        self.assertIn(
            'callbacks.reportError("mln_log_callback", err); return false; }', source
        )

    def test_a_status_callback_answers_a_named_status_and_reports_other_errors(self):
        source = zig.generate(real_api())["maplibre_native_ffi.zig"]
        self.assertIn(
            "url: []const u8, out_response: HttpHeaderTransformResponse) "
            "anyerror!void = null,",
            source,
        )
        # An error that names a status fails on purpose, so only another error
        # is reported, and it answers the failure.
        self.assertIn(
            "catch |err| { if (status.rawStatus(err)) |deliberate| return deliberate; "
            'callbacks.reportError("mln_http_header_transform_callback", err); '
            "return c.MLN_STATUS_NATIVE_ERROR; };",
            source,
        )

    def test_a_dispose_with_a_completion_releases_and_disposes(self):
        api = parse(
            """
typedef unsigned long long mln_session BIND("kind=handle;release=mln_session_dispose;dispose=mln_session_dispose");
BIND("execution=lifecycle")
mln_status mln_session_dispose(mln_session session, const mln_completion *completion, mln_diagnostic *out_diagnostic);
""",
            defines=("MLN_PROTOCOL_COMPLETION_RUNTIME",),
        )
        validate(api)
        self.assertEqual(zig.coverage(api)["unsupported"], {})
        source = zig.generate(api)["maplibre_native_ffi.zig"]
        # deinit and the sync dispose pass no completion; the release passes one.
        self.assertIn(
            "try status.call(c.mln_session_dispose, .{ raw, null }, null);", source
        )
        # Both take the caller's value and close it once native accepts. The
        # sync dispose of a closed value does nothing, and the async release
        # reaches native.
        self.assertIn(
            "pub fn release(self: *Session, diagnostic: ?*diagnostics.Diagnostic) status.Error!completion.Future(void) {\n"
            '    const released = try call.submit("mln_session_dispose", .handle, self.*,',
            source,
        )
        self.assertIn(
            "pub fn dispose(self: *Session, diagnostic: ?*diagnostics.Diagnostic) status.Error!void {\n"
            "    if (self.closed) return;\n"
            '    try call.discard("mln_session_dispose", .handle, self.*,',
            source,
        )
        self.assertEqual(source.count("self.closed = true;"), 2)

    def test_only_runtime_event_sources_document_their_id_as_a_source(self):
        api = parse(
            """
typedef enum mln_runtime_event_source_type : unsigned {
  MLN_RUNTIME_EVENT_SOURCE_MAP = 1,
} mln_runtime_event_source_type;
typedef unsigned long long mln_map BIND("kind=handle;release=mln_map_release");
typedef unsigned long long mln_query BIND("kind=handle;release=mln_query_release");
void mln_map_release(mln_map map);
void mln_query_release(mln_query query);
"""
        )
        validate(api)
        source = zig.generate(api)["maplibre_native_ffi.zig"]
        note = "/// Runtime events report its `id()` as their source.\n"
        self.assertIn(note + "pub const Map = struct {", source)
        self.assertNotIn(note + "pub const Query", source)

    def test_generated_values_and_keyword_parameters_round_trip(self):
        zig_tool = require_tool(self, "zig", BINDING)
        header = protocol_header(
            groups=(
                "caller_sized_output",
                "caller_sized_text",
                "values",
                "keywords",
                "defaults",
                "default_registration",
                "absent_handle",
                "absent_value",
                "owned_argument",
            ),
            defines=("MLN_PROTOCOL_COMPLETION_RUNTIME", "MLN_PROTOCOL_ABI_VERSION"),
        )
        api = parse_sources({"api.h": header})
        validate(api)
        self.assertEqual(zig.coverage(api)["unsupported"], {})
        with TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "api.h").write_text(header)
            source = root / "src"
            source.mkdir()
            for path, text in zig.generate(api).items():
                (source / path).write_text(text)
            for path in (BINDING / "src").glob("*.zig"):
                if not (source / path.name).exists() and path.name != "c.zig":
                    shutil.copy(path, source / path.name)
            shutil.copy(ROOT / "build.zig.zon", root)
            shutil.copy(FIXTURES / "probes/zig/build.zig", root)
            (source / "c.zig").write_text('pub const raw = @import("c_api");\n')
            shutil.copy(FIXTURES / "probes/zig/probe_test.zig", source)
            zig_tool.run(
                self,
                "build",
                "test",
                f"-Dfixture-dir={FIXTURES}",
                cwd=root,
                timeout=300,
                output=root / "zig-test.log",
            )


if __name__ == "__main__":
    unittest.main()
