"""Test generated Zig with the handwritten runtime against a C implementation."""

import shutil
import subprocess
import unittest
from pathlib import Path
from tempfile import TemporaryDirectory

from support import (
    FIXTURES,
    PROTOCOLS_STUB,
    ROOT,
    parse_sources,
    protocol_header,
    require_tool,
)

from tools.bindgen.emitters import zig
from tools.bindgen.schema import validate

BINDING = ROOT / "bindings/zig"


class ZigEmitterTests(unittest.TestCase):
    def test_generated_values_and_keyword_parameters_round_trip(self):
        zig_tool = require_tool(self, "zig", BINDING)
        header = protocol_header(
            groups=(
                "values",
                "keywords",
                "defaults",
                "default_registration",
                "absent_handle",
                "absent_value",
            ),
            defines=("MLN_PROTOCOL_COMPLETION_RUNTIME",),
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
            # translate-c stands in for the build's C module. It spins when
            # stdout is a pipe, so it writes to a file.
            with (source / "c_api.zig").open("w") as translated:
                result = subprocess.run(
                    [zig_tool.executable, "translate-c", str(root / "api.h")],
                    cwd=root,
                    env=zig_tool.env,
                    stdout=translated,
                    stderr=subprocess.PIPE,
                    text=True,
                    timeout=120,
                    check=False,
                )
            self.assertEqual(result.returncode, 0, result.stderr)
            (source / "c.zig").write_text('pub const raw = @import("c_api.zig");\n')
            shutil.copy(FIXTURES / "probes/zig/probe_test.zig", source)
            zig_tool.run(
                self,
                "test",
                str(source / "probe_test.zig"),
                str(PROTOCOLS_STUB),
                f"-I{FIXTURES}",
                "-lc",
                "--test-filter",
                "generated",
                cwd=root,
                timeout=300,
                output=root / "zig-test.log",
            )


if __name__ == "__main__":
    unittest.main()
