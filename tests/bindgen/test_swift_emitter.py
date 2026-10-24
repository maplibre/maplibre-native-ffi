"""Compile generated Swift with the handwritten runtime and run it against C."""

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
    require_tool,
    run,
)

from tools.bindgen.emitters import swift
from tools.bindgen.schema import validate

RUNTIME = ROOT / "bindings/swift/Sources/MaplibreNativeFFI"

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


class SwiftEmitterTests(unittest.TestCase):
    def test_a_dispose_with_a_completion_is_the_close_and_the_dispose(self):
        outputs = swift.generate(parse(DISPOSE_WITH_COMPLETION))
        operations = outputs["Generated/Operations/GeneratedApiLeaseOperations.swift"]
        # The close reports retirement through the completion, and the
        # dispose, like every disposer, passes none.
        self.assertEqual(operations.count("func close("), 1)
        self.assertIn("func close() async throws {", operations)
        self.assertIn(
            "nativeStartClose { raw, completion, diagnostic in "
            "mln_lease_dispose(raw, completion, diagnostic) }",
            operations,
        )
        self.assertIn(
            "func dispose() throws {\n"
            "  try nativeClose { raw, diagnostic in mln_lease_dispose(raw, nil, diagnostic) }",
            operations,
        )
        self.assertIn(
            "mln_lease_dispose(raw, nil, nil) == MLN_STATUS_OK",
            outputs["Generated/GeneratedOwners.swift"],
        )

    def test_a_first_label_that_repeats_the_method_name_is_omitted(self):
        # A trailing Id or Name names the key of what the method names.
        self.assertTrue(swift.repeats("setStyleUrl", "url"))
        self.assertTrue(swift.repeats("updateCamera", "update"))
        self.assertTrue(swift.repeats("deleteOfflineRegion", "regionId"))
        self.assertTrue(swift.repeats("setGlobalStateProperty", "propertyName"))
        self.assertFalse(swift.repeats("createMap", "options"))
        self.assertFalse(swift.repeats("setHeader", "name"))
        self.assertFalse(swift.repeats("queryFeatureExtensions", "sourceId"))

    def test_generated_values_and_keyword_parameters_round_trip(self):
        swiftc = require_tool(self, "swiftc", ROOT / "bindings/swift")
        clang = require_tool(self, "clang")
        header = protocol_header(
            groups=(
                "caller_sized_output",
                "caller_sized_text",
                "values",
                "defaults",
                "keywords",
                "default_registration",
                "absent_handle",
                "absent_value",
                "strided_records",
            ),
            defines=("MLN_PROTOCOL_COMPLETION_RUNTIME", "MLN_PROTOCOL_ABI_VERSION"),
        )
        api = parse_sources({"api.h": header})
        validate(api)
        self.assertEqual(swift.coverage(api)["unsupported"], {})
        with TemporaryDirectory() as directory:
            root = Path(directory)
            # Generated code imports the C declarations as MaplibreNativeC.
            (root / "include").mkdir()
            (root / "include/api.h").write_text(header)
            (root / "include/module.modulemap").write_text(
                'module MaplibreNativeC {\n  header "api.h"\n  export *\n}\n'
            )
            sources = []
            for path, text in swift.generate(api).items():
                sources.append(root / "generated" / path)
                sources[-1].parent.mkdir(parents=True, exist_ok=True)
                sources[-1].write_text(text)
            # Platform extensions extend the real API's generated records,
            # which the probe API lacks.
            for path in RUNTIME.rglob("*.swift"):
                if not {"Generated", "Platform"} & set(path.relative_to(RUNTIME).parts):
                    sources.append(path)
            sources.append(FIXTURES / "probes/swift/main.swift")
            stub = root / "protocols_stub.o"
            clang.run(
                self,
                "-std=c2x",
                "-c",
                "-fPIC",
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
                # The package builds the binding module with this feature,
                # which keeps an operation on its caller's executor until it
                # submits.
                "-enable-upcoming-feature",
                "NonisolatedNonsendingByDefault",
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
