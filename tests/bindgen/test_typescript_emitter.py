"""Check the TypeScript binding plan and run the generated addon under Node."""

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
    real_api,
    require_tool,
)

from tools.bindgen import async_hosts
from tools.bindgen.emitters import typescript
from tools.bindgen.schema import validate

BINDING = ROOT / "bindings/typescript"
PROBE = FIXTURES / "probes/typescript"

# The groups that protocols_stub.c implements, with the completion types that
# the addon runtime compiles against, and the groups that the probe's own
# results_stub.c implements.
GROUPS = (
    "caller_sized_output",
    "caller_sized_text",
    "values",
    "keywords",
    "defaults",
    "default_registration",
    "absent_handle",
    "absent_value",
    "strided_records",
    "map_close",
    "tagged_union",
    "completion_results",
)
DEFINES = ("MLN_PROTOCOL_COMPLETION_RUNTIME",)


class TypeScriptEmitterTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.api = real_api()
        cls.bound, cls.values, _, cls.lowered, _ = typescript.lower(cls.api)
        cls.coverage = typescript.coverage(cls.api)

    def test_each_operation_calls_its_own_function(self):
        # Each operation is one addon export for one C function. A fallible
        # function takes a diagnostic of its own call, and the export checks
        # its status. No call goes through a dispatch table of slots.
        names = [entry.plan.name for entry in self.lowered]
        self.assertEqual(len(names), len(set(names)))
        self.assertEqual(set(names), set(self.coverage["generated"]))
        self.assertEqual(self.coverage["unsupported"], {})
        for entry in self.lowered:
            with self.subTest(function=entry.plan.name):
                function = entry.plan.function
                self.assertIs(function, self.api.functions_by_name[entry.plan.name])
                self.assertEqual(entry.plan.status, bool(function.diagnostic))

    def test_registrations_reach_the_realm_through_the_shared_async_host_plan(
        self,
    ):
        # Every registration names the realm's delivery queue or posts a wake,
        # or leaves native rules to answer, as the Dart binding's do. No
        # registration needs a callback on a native thread.
        forms = {
            value.native: async_hosts.form(self.bound, value)
            for value in self.values.used.values()
            if value.kind == "record" and value.registration
        }
        self.assertEqual(
            forms,
            {
                "mln_camera_transition_handler": "queue",
                "mln_custom_geometry_source_options": "queue",
                "mln_custom_mvt_vector_source_options": "queue",
                "mln_http_header_transform": "rules",
                "mln_log_handler": "queue",
                "mln_queue_lock": "native",
                "mln_resource_provider": "queue",
                "mln_resource_request_cancel_handler": "queue",
                "mln_resource_transform": "queue",
                "mln_runtime_event_handler": "queue",
                "mln_wake": "wake",
            },
        )

    def test_native_registrations_carry_each_callback_then_the_context(self):
        # A wake and a registration that only native code can supply take a
        # native registration, whose fields mirror the C descriptor. A record
        # that holds one exposes it as an optional member.
        native = {
            value.native: [
                field.name for field in self.values.registration(value).native
            ]
            for value in self.values.used.values()
            if value.kind == "record"
            and value.registration
            and self.values.registration(value).native
        }
        self.assertEqual(
            native,
            {
                "mln_queue_lock": [
                    "lock",
                    "unlock",
                    "user_data",
                    "release_user_data",
                ],
                "mln_wake": ["callback", "user_data", "release_user_data"],
            },
        )
        options = self.bound.values["mln_render_session_attach_options"]
        members = {f.name: f for f in self.values.fields(options)}
        self.assertIn("queue_lock", members)
        self.assertTrue(self.values.member_optional(options, members["queue_lock"]))

    def test_a_dispose_that_is_also_the_release_closes_both_ways(self):
        # The session's release is its dispose with a completion, so close()
        # awaits retirement, while dispose(), `using`, and an adopted handle's
        # cleanup pass no completion.
        text = next(
            text
            for name, text in typescript.generate(self.api).items()
            if name.endswith("generated_operations.ts")
        )
        self.assertIn('defineDisposers(RenderSessionHandle, "close", "dispose");', text)
        self.assertIn(
            "native.mln_render_session_dispose(handle, token, completion)", text
        )
        self.assertIn(
            "native.mln_render_session_dispose(handle, token, undefined)", text
        )
        self.assertIn(
            "native.mln_render_session_dispose(disposed, undefined, undefined)",
            text,
        )

    def test_only_synchronous_categories_return_without_a_promise(self):
        # An operation that the headers complete later returns a Promise or an
        # attachment, so only the categories that return synchronously can
        # hold the calling thread.
        asynchronous = {
            "command",
            "query",
            "query_owned",
            "run",
            "close_async",
            "attach",
        }
        for entry in self.lowered:
            with self.subTest(function=entry.plan.name):
                self.assertEqual(
                    entry.shape in asynchronous, entry.plan.completion is not None
                )
        self.assertEqual(
            {e.plan.execution for e in self.lowered if e.shape not in asynchronous},
            {"event_batch", "immediate", "snapshot"},
        )

    def run_probe(self, script: str):
        """Builds the generated addon against the protocol stubs and runs
        `script` from the probe directory under Node."""
        node = require_tool(self, "node", BINDING)
        clang = require_tool(self, "clang")
        headers = BINDING / "node_modules/node-api-headers/include"
        self.assertTrue(
            (headers / "node_api.h").exists(),
            "install the TypeScript binding's packages for node-api-headers",
        )
        header = protocol_header(groups=GROUPS, defines=DEFINES)
        api = parse_sources({"api.h": header})
        validate(api)
        self.assertEqual(typescript.coverage(api)["unsupported"], {})
        with TemporaryDirectory() as directory:
            root = Path(directory)
            for name, text in typescript.generate(api).items():
                target = root / Path(name).name
                if "/src/" in name:
                    target = root / "src" / Path(name).name
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_text(text)
            (root / "api.h").write_text(header)
            shutil.copytree(PROBE / "maplibre_native_c", root / "maplibre_native_c")
            shutil.copy(PROBE / "maplibre_native_c.h", root)
            shutil.copytree(BINDING / "src/internal", root / "src/internal")
            shutil.copy(BINDING / "src/index.ts", root / "src")
            shutil.copy(PROBE / script, root / "src")
            (root / "src/package.json").write_text('{"type": "module"}\n')
            # The stub implements the same groups with the completion types.
            stub = root / "stub.c"
            stub.write_text(
                "".join(f"#define {define}\n" for define in DEFINES)
                + f'#include "{PROTOCOLS_STUB}"\n'
            )
            addon = root / "probe.node"
            clang.run(
                self,
                "-std=c11",
                "-shared",
                "-fPIC",
                "-Wall",
                "-Wextra",
                "-Wno-unused-parameter",
                "-Werror",
                "-DNAPI_VERSION=8",
                *(("-undefined", "dynamic_lookup") if sys.platform == "darwin" else ()),
                f"-I{root}",
                f"-I{FIXTURES}",
                f"-I{BINDING / 'native'}",
                f"-I{headers}",
                str(root / "generated_addon.c"),
                str(root / "generated_values.c"),
                str(BINDING / "native/realm.c"),
                str(BINDING / "native/runtime.c"),
                str(PROBE / "queue_stub.c"),
                str(PROBE / "results_stub.c"),
                str(stub),
                "-o",
                str(addon),
                cwd=root,
            )
            node.run(
                self,
                str(root / "src" / script),
                cwd=root,
                env={"MAPLIBRE_NATIVE_FFI_ADDON": str(addon)},
            )

    def test_protocol_probe_runs_under_node(self):
        self.run_probe("main.ts")

    def test_generated_decoders_keep_unknown_arms_and_read_at_the_reported_stride(
        self,
    ):
        self.run_probe("decoders.ts")


if __name__ == "__main__":
    unittest.main()
