"""Check the TypeScript binding plan and run the generated addon under Node."""

import re
import shutil
import sys
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
    "absent_handle",
    "absent_value",
    "strided_records",
    "map_close",
    "tagged_union",
    "completion_results",
)
DEFINES = ("MLN_PROTOCOL_COMPLETION_RUNTIME", "MLN_PROTOCOL_ABI_VERSION")

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


class TypeScriptEmitterTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.api = real_api()
        cls.bound, cls.values, _, cls.lowered, _ = typescript.lower_operations(cls.api)
        cls.coverage = typescript.lower(cls.api)

    def test_each_operation_calls_its_own_function(self):
        # Each operation is one addon export for one C function. A fallible
        # function takes a diagnostic of its own call, and the export checks
        # its status. No call goes through a dispatch table of slots.
        names = [entry.plan.name for entry in self.lowered]
        self.assertEqual(len(names), len(set(names)))
        self.assertEqual(set(names), set(self.coverage.generated))
        self.assertEqual(self.coverage.unsupported, {})
        for entry in self.lowered:
            with self.subTest(function=entry.plan.name):
                function = entry.plan.function
                self.assertIs(function, self.api.functions_by_name[entry.plan.name])
                self.assertEqual(entry.plan.status, bool(function.diagnostic))

    def test_registrations_reach_the_realm_through_the_shared_async_host_plan(
        self,
    ):
        # Every registration and wake names the realm's delivery queue, or
        # leaves native rules to answer. No registration needs a callback on a
        # native thread.
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
                "mln_custom_vector_source_options": "queue",
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

    def test_each_queued_callback_runs_through_a_trampoline_and_its_adapter(self):
        # The addon numbers each callback type's adapter, and the TypeScript
        # table lists the adapters in the same order. A trampoline whose call
        # reaches no host value releases the handle that the call owns.
        generated = emitted(typescript, self.api)
        header = generated["bindings/typescript/native/generated_addon.h"]
        operations = generated["bindings/typescript/src/generated_operations.ts"]
        constants = re.findall(r"MLN_TS_CALLBACK_(\w+) = (\d+),", header)
        self.assertEqual(
            [int(number) for _, number in constants], list(range(len(constants)))
        )
        table = operations[operations.index("defineCallbacks([") :]
        self.assertEqual(
            [f"MLN_{name}" for name, _ in constants],
            [name.upper() for name in re.findall(r"  // (mln_\w+)\n", table)],
        )
        values = generated["bindings/typescript/native/generated_values.c"]
        trampoline = values[
            values.index("void mln_ts_dispatch_resource_provider_callback(") :
        ]
        self.assertIn(
            "if (!received) {\n    mln_resource_request_release(handle);\n  }",
            trampoline[: trampoline.index("\n}\n")],
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
            for name, text in emitted(typescript, self.api).items()
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

    def test_a_record_of_only_a_union_is_that_union(self):
        # The union takes the record's name. Only a union that native returns
        # keeps an arm for a tag this binding predates.
        text = self.values.render()
        self.assertIn(
            "export type RenderedQueryGeometry =\n"
            '  | { readonly kind: "point"; readonly value: ScreenPoint }\n'
            '  | { readonly kind: "box"; readonly value: ScreenBox }\n'
            '  | { readonly kind: "lineString"; readonly value: ScreenLineString };\n',
            text,
        )
        definition = text[text.index("export type OfflineRegionDefinition =") :]
        self.assertIn(
            '| { readonly kind: "unknown"; readonly tag: number;',
            definition[: definition.index(";\n")],
        )
        self.assertNotIn("RenderedQueryGeometryData", text)
        self.assertNotIn("OfflineRegionDefinitionData", text)

    def test_only_event_sources_document_their_id_as_an_event_source(self):
        # Every owner inherits its ID, closed state, and debug text from
        # Owner. Only the handles that runtime events name redeclare the ID,
        # to document it as their source.
        text = next(
            text
            for name, text in emitted(typescript, self.api).items()
            if name.endswith("generated_operations.ts")
        )

        def owner(name):
            start = text.index(f"export class {name} extends Owner {{")
            return text[start : text.index("\n}\n", start)]

        for name in ("MapHandle", "RuntimeHandle"):
            self.assertIn("declare readonly id: bigint;", owner(name))
            self.assertIn("runtime events report as their", owner(name))
        for name in ("MapProjectionHandle", "RenderSessionHandle"):
            self.assertNotIn("readonly id", owner(name))

    def test_a_drain_returns_its_slice_and_releases_its_batch(self):
        outputs = emitted(
            typescript, parse(SLICE_DRAIN, defines=("MLN_PROTOCOL_STANDARD_TYPES",))
        )
        operations = outputs["bindings/typescript/src/generated_operations.ts"]
        # Nothing queued reads as an empty array.
        self.assertIn(
            "export function drainReadings(): readonly Reading[] {\n"
            "  return native.mln_drain_readings() ?? [];\n}",
            operations,
        )
        # The addon copies the slice, then releases the batch whether or not
        # the copy succeeded.
        addon = outputs["bindings/typescript/native/generated_addon.c"]
        self.assertIn(
            "call->result = mln_ts_read_reading_view(env, &(arg_out_view), sizeof(arg_out_view));\n"
            "  mln_reading_batch_release(arg_out_batch);\n",
            addon,
        )
        self.assertIn(
            "return mln_ts_read_reading_view_readings(env, source);",
            outputs["bindings/typescript/native/generated_values.c"],
        )
        # Neither the batch nor its view is public.
        self.assertNotIn("ReadingBatch", operations)
        self.assertNotIn(
            "ReadingView", outputs["bindings/typescript/src/generated_values.ts"]
        )

    def test_a_callback_receives_its_slice_argument_as_a_list(self):
        values = emitted(typescript, self.api)[
            "bindings/typescript/src/generated_values.ts"
        ]
        self.assertIn(
            "export type RuntimeEventHandler = (events: readonly RuntimeEvent[]) => void;",
            values,
        )
        self.assertNotIn("EventBatchView", values)

    def test_a_registration_without_its_queue_requires_each_non_nullable_callback(
        self,
    ):
        # The realm sets the delivery queue itself, so a host cannot leave out
        # a callback that C requires in callback mode. A callback that C marks
        # nullable stays optional.
        generated = emitted(typescript, self.api)
        values = generated["bindings/typescript/src/generated_values.ts"]
        operations = generated["bindings/typescript/src/generated_operations.ts"]
        self.assertIn(
            "setEventHandler(handler: RuntimeEventHandler): Promise<CommandCompletion>",
            operations,
        )
        self.assertNotIn("handler?: RuntimeEventHandler", operations)
        self.assertIn("  fetchTile: (tileId: CanonicalTileId) => void;", values)
        self.assertIn("  cancelTile?: (tileId: CanonicalTileId) => void;", values)
        self.assertIn("  endHandler?: CameraTransitionHandler;", values)

    def test_only_a_required_member_must_be_given(self):
        # A member that a host leaves out is zero, as native reads it, except
        # a required one, which makes its record a required argument too.
        generated = emitted(typescript, self.api)
        values = generated["bindings/typescript/src/generated_values.ts"]
        operations = generated["bindings/typescript/src/generated_operations.ts"]
        self.assertIn("  initialExtent: LogicalExtent;\n", values)
        self.assertIn("  mapMode?: MapMode;\n", values)
        self.assertIn("  assetPath?: string;\n", values)
        self.assertIn("createMap(options: MapOptions): NewMap", operations)
        self.assertIn(
            "  static create(options: RuntimeOptions = {}): Promise<RuntimeHandle> {",
            operations,
        )

    def test_a_call_that_waits_for_a_thread_start_runs_on_its_own_thread(self):
        # A browser page cannot wait for a thread to start, so the export
        # copies its converted arguments and returns, and the Promise settles
        # when the call's thread has created the owner.
        generated = emitted(typescript, self.api)
        operations = generated["bindings/typescript/src/generated_operations.ts"]
        addon = generated["bindings/typescript/native/generated_addon.c"]
        self.assertIn(
            "    return threadStart((start) => native.mln_runtime_create(options, start), ",
            operations,
        )
        self.assertIn("so a browser page, which cannot block,", operations)
        self.assertIn(
            "  mln_status status = mln_runtime_create("
            "&arguments->arg_options, &handle, diagnostic);\n",
            addon,
        )
        self.assertIn("  mln_runtime_dispose((mln_runtime)handle, NULL);\n", addon)
        self.assertIn(
            "  arguments.arg_options = arg_options;\n"
            "  mln_ts_start_thread(call, call->argv[1], mln_ts_run_mln_runtime_create, "
            "mln_ts_dispose_mln_runtime_create, &arguments, sizeof arguments);\n",
            addon,
        )
        self.assertNotIn("mln_runtime arg_out_runtime", addon)

    def test_a_factory_is_static_and_a_global_names_its_verb_first(self):
        operations = emitted(typescript, self.api)[
            "bindings/typescript/src/generated_operations.ts"
        ]
        runtime = operations[operations.index("export class RuntimeHandle ") :]
        runtime = runtime[: runtime.index("\n}\n")]
        self.assertIn("  static create(", runtime)
        self.assertNotIn("export function create(", operations)
        self.assertIn("export function setLogCallback(handler: LogHandler)", operations)

    def test_docs_name_the_binding_symbols_of_c_declarations(self):
        values = emitted(typescript, self.api)[
            "bindings/typescript/src/generated_values.ts"
        ]
        self.assertIn(
            "`MapHandle.cancelTransitions()`, `MapHandle.cancelCameraTransition()`, or\n"
            "   * `GesturePhase.cancel` ended the transitions",
            values,
        )
        self.assertNotIn("`mln_map_cancel_transitions()`", values)

    def test_only_synchronous_categories_return_without_a_promise(self):
        # An operation that the headers complete later returns a Promise or an
        # attachment, and one that waits for a thread to start runs on a thread
        # of its own, so only the categories that return synchronously can
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
                    entry.shape == "thread_start", entry.plan.blocks is not None
                )
        synchronous = [
            e
            for e in self.lowered
            if e.shape not in asynchronous and e.shape != "thread_start"
        ]
        self.assertEqual(
            {e.plan.execution for e in synchronous},
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
        self.assertEqual(typescript.lower(api).unsupported, {})
        with TemporaryDirectory() as directory:
            root = Path(directory)
            for name, text in emitted(typescript, api).items():
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
