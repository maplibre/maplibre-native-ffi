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
    parse_sources,
    protocol_header,
    require_tool,
)

from tools.bindgen.emitters import dart
from tools.bindgen.schema import validate

BINDING = ROOT / "bindings/dart"


class DartEmitterTests(unittest.TestCase):
    def test_generated_values_and_keyword_parameters_round_trip(self):
        dart_tool = require_tool(self, "dart", BINDING)
        clang = require_tool(self, "clang")
        header = protocol_header(groups=("values", "keywords"))
        api = parse_sources({"api.h": header})
        validate(api)
        self.assertEqual(
            set(dart.coverage(api)["generated"]),
            {"mln_keyword_combine", "mln_probe_roundtrip"},
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
            # The probe groups name no handle owners, so the probe's values
            # library takes the binding's without its runtime import.
            (root / "values.dart").write_text(
                "".join(
                    line
                    for line in (BINDING / "lib/src/values.dart")
                    .read_text()
                    .splitlines(keepends=True)
                    if "runtime/runtime.dart" not in line
                )
            )
            (root / "generated_values.dart").write_text(dart.generate_values(api))
            (root / "generated_operations.dart").write_text(dart.generate(api))
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
