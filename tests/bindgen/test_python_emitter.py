"""Exercise header-driven Python values and identifier isolation."""

import enum
import inspect
import sys
import types
import typing
import unittest
from unittest.mock import MagicMock, patch

from support import parse, real_api

from tools.bindgen.emitters import python
from tools.bindgen.schema import validate


class PythonEmitterTests(unittest.TestCase):
    def parse(self, source="", groups=()):
        api = parse(source, groups=groups)
        validate(api)
        return api

    def materialize(self, api, native=None):
        """Execute the generated modules over fakes of the handwritten runtime."""
        files = python.generate(api)
        for path, source in files.items():
            if path.endswith((".py", ".pyi")):
                compile(source, path, "exec")

        def module(name, **attributes):
            result = types.ModuleType(f"fixture.{name}")
            result.__package__ = "fixture"
            result.__dict__.update(attributes)
            return result

        class GeneratedOperations:
            pass

        class NativeHandleMixin:
            pass

        class ScopedHandleMixin(NativeHandleMixin):
            pass

        modules = {
            "fixture": types.ModuleType("fixture"),
            "fixture._enum": module("_enum", UnknownIntEnum=enum.IntEnum),
            "fixture._native": native or MagicMock(),
            "fixture._completion": module("_completion", CommandCompletion=object),
            "fixture._future": module("_future", map_future=None),
            "fixture._operation": module(
                "_operation",
                GeneratedOperations=GeneratedOperations,
                _adopt_value=None,
                _view=None,
            ),
            "fixture._lifecycle": module(
                "_lifecycle",
                NativeHandleMixin=NativeHandleMixin,
                ScopedHandleMixin=ScopedHandleMixin,
            ),
        }
        generated = types.SimpleNamespace(NativeHandleMixin=NativeHandleMixin)
        with patch.dict(sys.modules, modules):
            for name in ("values", "operations", "owners"):
                loaded = module(f"_generated_{name}")
                sys.modules[loaded.__name__] = loaded
                exec(  # noqa: S102 -- Exercise the generated modules.
                    files[f"python/maplibre_native_ffi/_generated_{name}.py"],
                    loaded.__dict__,
                )
                setattr(generated, name, loaded)
        return generated

    def test_nested_copied_values_are_deep_and_escape_keyword_fields(self):
        api = self.parse("""
typedef struct mln_position { double latitude; double longitude; } mln_position;
typedef struct mln_new_entry {
  mln_buffer_view title;
  mln_position position;
  double class;
} mln_new_entry;
BIND("execution=query;result=mln_new_entry;shape=array")
mln_status mln_map_new_entries(mln_map map, const mln_completion *completion, mln_diagnostic *out_diagnostic);
""")
        self.assertEqual(python.coverage(api)["generated"], ["mln_map_new_entries"])
        values = self.materialize(api).values
        raw = {
            "title": "Harbor",
            "position": {"latitude": 3.25, "longitude": -8.5},
            "class_": 2.0,
        }
        copied = values.NewEntry._from_native(raw)
        raw["position"]["latitude"] = 0
        self.assertEqual(copied.position, values.Position(3.25, -8.5))
        self.assertEqual(copied.title, "Harbor")
        self.assertEqual(copied.class_, 2.0)

    def test_only_a_required_member_lacks_a_zero_default(self):
        api = self.parse("""
typedef struct mln_probe_route_options {
  unsigned size;
  mln_buffer_view url BIND("required=true");
  mln_buffer_view label;
  unsigned weight;
} mln_probe_route_options;
typedef struct mln_probe_label_options {
  unsigned size;
  mln_buffer_view label;
} mln_probe_label_options;
mln_status mln_probe_route_add(
  const mln_probe_route_options* options, mln_diagnostic* out_diagnostic
);
mln_status mln_probe_label_set(
  const mln_probe_label_options* options, mln_diagnostic* out_diagnostic
);
""")
        generated = self.materialize(api)
        values = generated.values
        with self.assertRaises(TypeError):
            values.ProbeRouteOptions()
        self.assertEqual(
            values.ProbeRouteOptions(url="/tiles"),
            values.ProbeRouteOptions(url="/tiles", label="", weight=0),
        )
        # An options record with a required member is itself required.
        operations = generated.operations
        route = inspect.signature(operations.probe_route_add).parameters["options"]
        label = inspect.signature(operations.probe_label_set).parameters["options"]
        self.assertIs(route.default, inspect.Parameter.empty)
        self.assertIsNone(label.default)

    def test_a_nested_registration_defaults_to_a_disabled_one(self):
        api = self.parse(groups=("record_notification",))
        self.assertIn("mln_map_request_reading", python.coverage(api)["generated"])
        values = self.materialize(api).values
        first = values.ReadingRequest(level=3)
        second = values.ReadingRequest(level=4)
        self.assertIsNone(first.handler.callback)
        self.assertIsNot(first.handler, second.handler)

    def test_wakes_and_synchronous_registrations_take_a_native_registration(self):
        api = self.parse(
            """
BIND("synchronous=true")
typedef void (*mln_probe_guard_callback)(void* user_data);
typedef struct mln_probe_guard {
  mln_probe_guard_callback enter BIND("nullable=true");
  mln_probe_guard_callback leave BIND("nullable=true");
  void* user_data BIND("kind=context");
  mln_probe_notify_release release_user_data;
} mln_probe_guard BIND("kind=callback_registration;release=release_user_data");
mln_status mln_probe_hooks_install(
  const mln_probe_hooks* hooks, const mln_probe_guard* guard,
  mln_diagnostic* out_diagnostic
);
""",
            groups=("nested_registration", "record_notification"),
        )
        self.assertIn("mln_probe_hooks_install", python.coverage(api)["generated"])
        native = types.SimpleNamespace(_NativeRegistration=lambda *parts: parts)
        values = self.materialize(api, native).values
        package = types.ModuleType("fixture")
        package._native = native
        with patch.dict(sys.modules, {"fixture": package, "fixture._native": native}):
            wake = values.ProbeSignal.native(callback=5, user_data=6)
            guard = values.ProbeGuard.native(enter=1, leave=2, release_user_data=3)
        # Each address reaches the runtime unchanged, in field order.
        self.assertIsInstance(wake, values.NativeProbeSignal)
        self.assertEqual(wake._registration, ("mln_probe_signal", (5,), 6, 0))
        self.assertEqual(guard._registration, ("mln_probe_guard", (1, 2), 0, 3))
        # A record field takes either form.
        self.assertEqual(
            typing.get_type_hints(values.ProbeHooks, vars(values))["signal"],
            values.ProbeSignal | values.NativeProbeSignal,
        )
        with self.assertRaises(TypeError):
            values.NativeProbeSignal()
        # A registration with a queue delivery takes only host callbacks.
        self.assertFalse(hasattr(values.ReadingHandler, "native"))

    def test_input_identifiers_cannot_shadow_callback_runtime_locals(self):
        api = self.parse("""
BIND("execution=command")
mln_status mln_map_match(mln_map map, double self, double input_self, double py,
                       mln_buffer_view title, double title_view,
                       const mln_completion *completion, mln_diagnostic *out_diagnostic);
""")
        self.assertEqual(python.coverage(api)["generated"], ["mln_map_match"])
        native = MagicMock()
        operations = self.materialize(api, native).operations
        operations.map_match(7, 1.0, 2.0, 3.0, "title", 4.0)
        native.map_match.assert_called_once_with(7, 1.0, 2.0, 3.0, "title", 4.0)

    def test_escaped_field_collisions_fail_before_generating_an_api(self):
        api = self.parse("""
typedef struct mln_entry { double class; double class_; } mln_entry;
BIND("execution=query;result=mln_entry")
mln_status mln_map_entry(mln_map map, const mln_completion *completion, mln_diagnostic *out_diagnostic);
""")
        result = python.coverage(api)
        self.assertEqual(result["generated"], [])
        self.assertIn("collide", result["unsupported"]["mln_map_entry"])

    def test_operations_cannot_take_owner_member_names(self):
        api = self.parse("""
typedef unsigned long long mln_host BIND("kind=handle;release=mln_host_destroy");
mln_status mln_host_destroy(mln_host host, mln_diagnostic *out_diagnostic);
mln_status mln_host_id(mln_host host, bool *out BIND("direction=out"), mln_diagnostic *out_diagnostic);
""")
        result = python.coverage(api)
        # The release alone becomes close.
        self.assertEqual(result["generated"], ["mln_host_destroy"])
        self.assertIn("owner member", result["unsupported"]["mln_host_id"])

    def test_a_factory_is_a_classmethod_and_a_global_names_its_verb_first(self):
        api = self.parse("""
typedef unsigned long long mln_engine BIND("kind=handle;release=mln_engine_release;dispose=mln_engine_release");
void mln_engine_release(mln_engine engine);
mln_status mln_engine_create(mln_engine *out_engine BIND("direction=out"), mln_diagnostic *out_diagnostic);
mln_status mln_level_set_volume(unsigned volume, mln_diagnostic *out_diagnostic);
""")
        self.assertEqual(python.coverage(api)["unsupported"], {})
        files = python.generate(api)
        native = files["src/generated_operations.rs"]
        engine = native.split("\nimpl EngineHandle {\n", 1)[1].split("\n}\n", 1)[0]
        self.assertIn(
            "#[staticmethod]\n    #[pyo3(signature = ())]\n    fn create(", engine
        )
        self.assertIn("wrap_pyfunction!(set_level_volume, module)", native)
        self.assertNotIn("wrap_pyfunction!(create, module)", native)
        operations = files["python/maplibre_native_ffi/_generated_operations.py"]
        self.assertIn(
            "    @classmethod\n    def create(cls) -> EngineHandle:", operations
        )
        self.assertIn("_native._EngineHandle.create()", operations)
        self.assertIn("\ndef set_level_volume(volume: int) -> None:", operations)
        exports = files["python/maplibre_native_ffi/api.py"]
        self.assertIn("from ._generated_operations import set_level_volume", exports)
        self.assertNotIn("import create", exports)

    def test_docs_name_the_python_symbols_of_c_declarations(self):
        values = python.generate(real_api())[
            "python/maplibre_native_ffi/_generated_values.py"
        ]
        self.assertIn("    `MapHandle.cancel_camera_transition()` matches.\n", values)
        self.assertNotIn("`mln_map_cancel_camera_transition()`", values)

    def test_scoped_exit_disposes_sessions_and_roots_and_skips_a_closed_handle(self):
        native = python.generate(real_api())["src/generated_operations.rs"]

        def method(owner, name):
            block = native.split(f"\nimpl {owner} {{\n", 1)[1].split("\n}\n", 1)[0]
            return block.split(f"fn {name}(", 1)[1].split("#[pyo3", 1)[0]

        # A session's dispose is also its release: its explicit close passes
        # a completion, and its scope passes none.
        self.assertIn(
            "mln_render_session_dispose(handle, std::ptr::null(), diagnostic)",
            method("RenderSessionHandle", "_exit"),
        )
        self.assertIn(
            "mln_render_session_dispose(handle, completion, diagnostic)",
            method("RenderSessionHandle", "close"),
        )
        # A release that reports teardown through a completion would hand the
        # scope a future it drops, so the scope disposes the map instead.
        self.assertIn("mln_map_dispose", method("MapHandle", "_exit"))
        self.assertIn("mln_runtime_dispose", method("RuntimeHandle", "_exit"))
        self.assertIn("mln_map_release", method("MapHandle", "close"))
        # A scope that exits after an explicit asynchronous close does nothing,
        # while a repeated explicit close still reaches native.
        self.assertIn("self.reserve(true)", method("MapHandle", "_exit"))
        self.assertIn("self.reserve(false)", method("MapHandle", "close"))

    def test_a_dispose_that_is_the_release_closes_with_a_completion(self):
        api = self.parse("""
typedef unsigned long long mln_lease BIND("kind=handle;release=mln_lease_dispose;dispose=mln_lease_dispose");
BIND("execution=lifecycle")
mln_status mln_lease_dispose(mln_lease lease, const mln_completion *completion, mln_diagnostic *out_diagnostic);
""")
        self.assertEqual(python.coverage(api)["generated"], ["mln_lease_dispose"])
        files = python.generate(api)
        native = files["src/generated_operations.rs"]
        block = native.split("\nimpl LeaseHandle {\n", 1)[1].split("\n}\n", 1)[0]
        close = block.split("fn close(", 1)[1].split("#[pyo3", 1)[0]
        exit = block.split("fn _exit(", 1)[1].split("#[pyo3", 1)[0]
        # The close returns a future; the scope and every disposer pass a
        # null completion.
        self.assertIn("mln_lease_dispose(handle, completion, diagnostic)", close)
        self.assertIn("mln_lease_dispose(handle, std::ptr::null(), diagnostic)", exit)
        self.assertIn(
            "mln_lease_dispose(handle, std::ptr::null(), std::ptr::null_mut())",
            native.split("fn generated_dispose_mln_lease(", 1)[1].split("\n}", 1)[0],
        )
        self.assertIn(
            "def close(self) -> Future[None]:",
            files["python/maplibre_native_ffi/_generated_operations.py"],
        )
        # The synchronous dispose is the scope's exit.
        owners = files["python/maplibre_native_ffi/_generated_owners.py"]
        self.assertIn("def dispose(self) -> None:", owners)
        self.assertIn("self._native._exit()", owners)

    def test_dispose_is_public_only_where_c_has_one_besides_a_void_release(self):
        owners = python.generate(real_api())[
            "python/maplibre_native_ffi/_generated_owners.py"
        ]
        classes = {
            block.split("(", 1)[0]: block
            for block in owners.split("\nclass ")[1:]
            if "_handle_name" in block
        }
        self.assertEqual(len(classes), 10)
        self.assertEqual(
            {name for name, block in classes.items() if "def dispose(" in block},
            {"MapHandle", "RenderSessionHandle", "RuntimeHandle"},
        )
        # Every handle has an id, and only the runtime and the map are event
        # sources.
        self.assertTrue(all("def id(self) -> int:" in b for b in classes.values()))
        self.assertEqual(
            {
                name
                for name, block in classes.items()
                if "report as their source" in block
            },
            {"MapHandle", "RuntimeHandle"},
        )

    def test_a_record_of_only_a_union_is_its_variants(self):
        api = self.parse(groups=("tagged_union",))
        generated = self.materialize(api)
        operations, values = generated.operations, generated.values
        # A returned union keeps an arm for a tag this binding predates.
        self.assertEqual(
            values.Event.__value__, values.EventFrameVariant | values.UnknownVariant
        )
        operations.map_future = lambda future, convert: convert(future)
        native = operations._native
        native.map_event.return_value = {
            "kind": "frame",
            "value": {"timestamp": 12.5},
        }
        self.assertEqual(
            operations.map_event(1), values.EventFrameVariant(values.EventFrame(12.5))
        )
        native.map_event.return_value = {"kind": None, "tag": 42}
        self.assertEqual(operations.map_event(1), values.UnknownVariant(42))
        rust = python.generate(api)["src/generated_operations.rs"]
        self.assertIn("Ok(match value.kind {", rust)

    def test_an_input_only_union_has_no_unknown_arm(self):
        api = self.parse(
            """
typedef enum mln_shape_kind { MLN_SHAPE_DOT = 1, MLN_SHAPE_SPAN = 2 } mln_shape_kind;
typedef struct mln_dot { double x; } mln_dot;
typedef struct mln_span { double start; double end; } mln_span;
typedef struct mln_shape {
  uint32_t size;
  mln_shape_kind kind;
  union {
    mln_dot dot BIND("variant=MLN_SHAPE_DOT");
    mln_span span BIND("variant=MLN_SHAPE_SPAN");
  } data BIND("tag=kind");
} mln_shape;
mln_status mln_shape_draw(const mln_shape* shape, mln_diagnostic* out_diagnostic);
""",
            groups=("standard_types",),
        )
        values = self.materialize(api).values
        self.assertEqual(
            values.Shape.__value__, values.ShapeDotVariant | values.ShapeSpanVariant
        )
        rust = python.generate(api)["src/generated_operations.rs"]
        # The selected variant is the value, so its tag is read from it.
        self.assertIn('let field = value;\n    let tag = field.getattr("_tag")?', rust)

    def test_an_optional_member_that_presents_a_union_decodes_both_states(self):
        api = self.parse(
            """
typedef enum mln_shape_kind { MLN_SHAPE_DOT = 1 } mln_shape_kind;
typedef struct mln_dot { double x; } mln_dot;
typedef struct mln_shape {
  mln_shape_kind kind;
  union { mln_dot dot BIND("variant=MLN_SHAPE_DOT"); } data BIND("tag=kind");
} mln_shape;
typedef enum BIND("kind=bitmask") mln_scene_field : uint32_t { MLN_SCENE_SHAPE = 1 } mln_scene_field;
typedef uint32_t mln_scene_fields;
typedef struct mln_scene {
  mln_scene_fields fields;
  mln_shape shape BIND("mask=fields;bit=MLN_SCENE_SHAPE");
} mln_scene;
BIND("execution=query;result=mln_scene")
mln_status mln_map_scene(mln_map map, const mln_completion *completion, mln_diagnostic *out_diagnostic);
""",
            groups=("standard_types",),
        )
        values = self.materialize(api).values
        absent = values.Scene._from_native({"shape": None})
        self.assertIsNone(absent.shape)
        present = values.Scene._from_native(
            {"shape": {"kind": "dot", "value": {"x": 2.5}}}
        )
        self.assertEqual(present.shape, values.ShapeDotVariant(values.Dot(2.5)))

    def test_a_parameter_keeps_its_c_name_when_the_glue_renames_its_local(self):
        api = self.parse(
            """
mln_status mln_set_level(int32_t state, int32_t status, mln_diagnostic *out_diagnostic);
""",
            groups=("standard_types",),
        )
        generated = python.generate(api)
        facade = generated["python/maplibre_native_ffi/_generated_operations.py"]
        self.assertIn("def set_level(state: int, status: int) -> None:", facade)
        self.assertIn("_native.set_level(state, status)", facade)
        # The Rust glue renames its own locals, which the facade never shows.
        self.assertIn("input_state: i32", generated["src/generated_operations.rs"])

    def test_an_owned_callback_handle_arrives_as_its_owner(self):
        api = self.parse(groups=("owned_argument",))
        self.assertEqual(python.coverage(api)["unsupported"], {})
        generated = self.materialize(api)
        owners, values = generated.owners, generated.values
        # The handler hands its callback the received handle's owner.
        values._wrap_response = lambda native, owner: (owner, native)
        received = []
        handler = values.QueryHandler(
            callback=lambda text, query: received.append((text, query))
        )
        handler._invoke_callback("text", 41)
        self.assertEqual(received, [("text", ("QueryHandle", 41))])
        self.assertTrue(issubclass(owners.QueryHandle, generated.NativeHandleMixin))
        with self.assertRaises(TypeError):
            owners.QueryHandle()
