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

from tools.bindgen.compiler import compile_api
from tools.bindgen.emitters import dart, dart_native
from tools.bindgen.schema import validate

BINDING = ROOT / "bindings/dart"


class DartEmitterTests(unittest.TestCase):
    def test_generated_values_and_keyword_parameters_round_trip(self):
        dart_tool = require_tool(self, "dart", BINDING)
        clang = require_tool(self, "clang")
        header = protocol_header(
            groups=(
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
            # The probe groups name no handle owners, so an empty library
            # stands in for the runtime that the binding's values import.
            (root / "runtime").mkdir()
            (root / "runtime/runtime.dart").write_text("library;\n")
            (root / "generated_values.dart").write_text(dart.generate_values(api))
            (root / "generated_operations.dart").write_text(dart.generate(api))
            (root / "maplibre_native_c.g.dart").write_text(
                dart_native.generate(compile_api(api))
            )
            for name in ("native_abi.dart", "native_asset.dart"):
                shutil.copy(BINDING / "lib/src/internal/c" / name, root)
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
