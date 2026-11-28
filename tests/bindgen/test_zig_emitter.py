"""Test generated Zig with the handwritten runtime against a C implementation."""

import shutil
import unittest
from dataclasses import replace
from pathlib import Path
from tempfile import TemporaryDirectory

from support import (
    FIXTURES,
    ROOT,
    emitted,
    parse_sources,
    protocol_header,
    require_tool,
)

from tools.bindgen.emitters import zig
from tools.bindgen.model import Unsupported
from tools.bindgen.schema import validate

BINDING = ROOT / "bindings/zig"


class ZigEmitterTests(unittest.TestCase):
    def test_generated_values_and_keyword_parameters_round_trip(self):
        zig_tool = require_tool(self, "zig", BINDING)
        header = protocol_header(
            groups=(
                "caller_sized_output",
                "caller_sized_text",
                "values",
                "keywords",
                "defaults",
                "absent_handle",
                "owned_argument",
                "counted_inline_array",
            ),
            defines=("MLN_PROTOCOL_COMPLETION_RUNTIME", "MLN_PROTOCOL_ABI_VERSION"),
        )
        api = parse_sources({"api.h": header})
        validate(api)
        with TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "api.h").write_text(header)
            source = root / "src"
            source.mkdir()
            for path, text in emitted(zig, api).items():
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

    def test_rejects_a_struct_argument_that_zig_miscompiles_on_loongarch(self):
        header = """#include <stdint.h>
struct packed { uint8_t a; uint32_t b; };
struct widening { uint32_t a; uint64_t b; };
void take_packed(struct packed value);
void take_widening(struct widening value);
"""
        api = parse_sources({"api.h": header})
        packed = replace(api, functions=(api.functions_by_name["take_packed"],))
        self.assertIn("pub fn take_packed(", zig.abi_wrappers(packed))
        with self.assertRaisesRegex(Unsupported, "take_widening.*LoongArch"):
            zig.abi_wrappers(api)


if __name__ == "__main__":
    unittest.main()
