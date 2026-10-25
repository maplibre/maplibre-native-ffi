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
    require_tool,
)

from tools.bindgen.emitters import zig
from tools.bindgen.schema import validate

BINDING = ROOT / "bindings/zig"


class ZigEmitterTests(unittest.TestCase):
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
