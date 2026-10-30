"""Compile generated Swift with the handwritten runtime and run it against C."""

import unittest
from pathlib import Path
from tempfile import TemporaryDirectory

from support import (
    FIXTURES,
    PROTOCOLS_STUB,
    ROOT,
    emitted,
    parse,
    parse_sources,
    protocol_header,
    real_api,
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


class SwiftEmitterTests(unittest.TestCase):
    def test_android_exposes_initialization_and_vulkan_operations(self):
        outputs = emitted(swift, real_api())
        android = outputs[
            "Generated/Operations/GeneratedAndroidGlobalsOperations.swift"
        ]
        self.assertIn("#if os(Android)", android)
        self.assertIn("public func `initAndroid`", android)
        for group in (
            "SurfaceMap",
            "SurfaceRenderSession",
            "TextureMap",
            "TextureRenderSession",
            "TextureAcquiredFrame",
        ):
            operations = outputs[
                f"Generated/Operations/Generated{group}Operations.swift"
            ]
            self.assertIn("#if os(Android) || os(Linux) || os(macOS)", operations)

    def test_every_owner_has_the_same_handle_members(self):
        owners = emitted(swift, parse(EVENT_SOURCE_AND_LEASE))[
            "Generated/GeneratedOwners.swift"
        ]
        for owner in ("MapHandle", "LeaseHandle"):
            self.assertIn(
                f"public final class {owner}: @unchecked Sendable, NativeReceiver, "
                "Identifiable, Hashable, CustomStringConvertible {",
                owners,
            )
        self.assertEqual(owners.count("public var isClosed: Bool"), 2)
        self.assertEqual(
            owners.count("public var description: String { handle.description }"),
            2,
        )
        # Only an event source's id doc names events.
        self.assertEqual(owners.count("runtime events report"), 1)
        self.assertIn("runtime events report", owners.split("class MapHandle")[1])
        self.assertIn(
            "  /// The native handle, which no later handle in the process reuses.\n"
            "  /// It stays valid after the handle closes.\n  public var id: UInt64",
            owners,
        )

    def test_a_dispose_with_a_completion_is_the_close_and_the_dispose(self):
        outputs = emitted(swift, parse(DISPOSE_WITH_COMPLETION))
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

    def test_a_required_member_has_no_default_and_others_default_to_zero(self):
        outputs = emitted(swift, real_api())
        values = "".join(
            text for path, text in outputs.items() if path.startswith("Generated/")
        )
        self.assertIn(
            "public init(`initialExtent`: LogicalExtent, `mapMode`: MapMode = "
            ".init(rawValue: 0), `fastPforEnabled`: Bool = false, `eventMask`: "
            "RuntimeEventFlag? = nil)",
            values,
        )
        self.assertNotIn("static var `default`", values)
        # A call that returns a value cannot fail, and reading the C version
        # skips the ABI version check, which reads it.
        words = " ".join(values.split())
        self.assertIn("public func `cVersion`() -> UInt32 { mln_c_version() }", words)
        self.assertIn(
            "public func `supportedRenderBackendMask`() -> RenderBackendFlag { "
            "nativeDirect {",
            words,
        )

    def test_a_root_constructor_is_a_factory_and_a_global_names_its_verb_first(
        self,
    ):
        outputs = emitted(swift, real_api())
        runtime = outputs[
            "Generated/Operations/GeneratedRuntimeRuntimeOperations.swift"
        ]
        self.assertIn(
            "static func `create`(`options` bindingArg0: RuntimeOptions) throws "
            "-> RuntimeHandle {",
            runtime.split("public extension RuntimeHandle {")[1],
        )
        # A process global is a function of the module, not a type's member.
        logging = outputs[
            "Generated/Operations/GeneratedLoggingGlobalsOperations.swift"
        ]
        self.assertIn("\npublic func `clearLogCallback`() throws {", logging)
        self.assertNotIn("extension", logging)

    def test_a_record_of_only_a_tag_and_its_union_is_the_union(self):
        values = "".join(
            text
            for path, text in emitted(swift, real_api()).items()
            if path.startswith("Generated/Values/")
        )
        self.assertNotIn("RenderedQueryGeometryData", values)
        geometry = values.split("public enum RenderedQueryGeometry:")[1].split("\n}")[0]
        self.assertIn("case `point`(ScreenPoint)", geometry)
        # Native never returns a query geometry, so it has no unknown case; it
        # returns a region definition, which keeps one.
        self.assertNotIn("unknown", geometry)
        definition = values.split("public enum OfflineRegionDefinition:")[1]
        self.assertIn("case unknown(UInt32, Data)", definition.split("\n}")[0])

    def test_a_summary_names_the_swift_symbols_of_c_declarations(self):
        values = emitted(swift, real_api())["Generated/Values/GeneratedMapValues.swift"]
        summary = " ".join(
            line.strip().removeprefix("///").strip()
            for line in values.split(
                "public static let `cancelled`: CameraTransitionOutcome"
            )[0]
            .rsplit("public static let", 1)[1]
            .splitlines()[1:]
        )
        self.assertIn(
            "`MapHandle.cancelTransitions()`, `MapHandle.cancelCameraTransition()`, "
            "or `GesturePhase.cancel` ended the transitions",
            summary,
        )
        self.assertNotIn("mln_map_cancel_transitions", summary)

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
                "absent_handle",
                "absent_value",
                "strided_records",
            ),
            defines=("MLN_PROTOCOL_COMPLETION_RUNTIME", "MLN_PROTOCOL_ABI_VERSION"),
        )
        api = parse_sources({"api.h": header})
        validate(api)
        self.assertEqual(swift.lower(api).unsupported, {})
        with TemporaryDirectory() as directory:
            root = Path(directory)
            # Generated code imports the C declarations as MaplibreNativeC.
            (root / "include").mkdir()
            (root / "include/api.h").write_text(header)
            (root / "include/module.modulemap").write_text(
                'module MaplibreNativeC {\n  header "api.h"\n  export *\n}\n'
            )
            sources = []
            for path, text in emitted(swift, api).items():
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
                # A factory calls the runtime's global helpers by this module's
                # name, since its handle's own helpers shadow them.
                "-module-name",
                "MaplibreNativeFFI",
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
