"""Compile generated Swift with the handwritten runtime and run it against C."""

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
    run,
)

from tools.bindgen.emitters import swift
from tools.bindgen.schema import validate

RUNTIME = ROOT / "bindings/swift/Sources/MaplibreNativeFFI"


class SwiftEmitterTests(unittest.TestCase):
    def test_generated_values_and_keyword_parameters_round_trip(self):
        swiftc = require_tool(self, "swiftc", ROOT / "bindings/swift")
        clang = require_tool(self, "clang")
        header = protocol_header(
            groups=("values", "keywords"),
            defines=("MLN_PROTOCOL_COMPLETION_RUNTIME", "MLN_PROTOCOL_ABI_VERSION"),
        )
        api = parse_sources({"api.h": header})
        validate(api)
        self.assertEqual(swift.coverage(api)["unsupported"], {})
        with TemporaryDirectory() as directory:
            root = Path(directory)
            # Generated code imports the C declarations as CMaplibreNativeC.
            (root / "include").mkdir()
            (root / "include/api.h").write_text(header)
            (root / "include/module.modulemap").write_text(
                'module CMaplibreNativeC {\n  header "api.h"\n  export *\n}\n'
            )
            sources = []
            for path, text in swift.generate(api).items():
                sources.append(root / "generated" / path)
                sources[-1].parent.mkdir(parents=True, exist_ok=True)
                sources[-1].write_text(text)
            for path in RUNTIME.rglob("*.swift"):
                if "Generated" not in path.relative_to(RUNTIME).parts:
                    sources.append(path)
            sources.append(FIXTURES / "probes/swift/main.swift")
            stub = root / "protocols_stub.o"
            clang.run(
                self,
                "-std=c2x",
                "-c",
                f"-I{FIXTURES}",
                str(PROTOCOLS_STUB),
                "-o",
                str(stub),
                cwd=root,
            )
            swiftc.run(
                self,
                "-swift-version",
                "6",
                "-module-name",
                "Probe",
                "-I",
                str(root / "include"),
                *map(str, sources),
                str(stub),
                "-o",
                str(root / "probe"),
                cwd=root,
                timeout=300,
            )
            run(self, [str(root / "probe")], root)
            run(self, [str(root / "probe")], root, env={"MLN_PROBE_C_VERSION": "1"})


if __name__ == "__main__":
    unittest.main()
