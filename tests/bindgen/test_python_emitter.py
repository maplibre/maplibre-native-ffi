"""Exercise header-driven Python values and identifier isolation."""

import enum
import sys
import types
import unittest
from unittest.mock import patch

from support import parse

from tools.bindgen.emitters import python
from tools.bindgen.schema import validate


class PythonEmitterTests(unittest.TestCase):
    def parse(self, source="", groups=()):
        api = parse(source, groups=groups)
        validate(api)
        return api

    def materialize(self, api):
        files = python.generate(api)
        for path, source in files.items():
            if path.endswith((".py", ".pyi")):
                compile(source, path, "exec")
        package = types.ModuleType("fixture")
        enums = types.ModuleType("fixture._enum")
        enums.UnknownIntEnum = enum.IntEnum
        values = types.ModuleType("fixture.values")
        values.__package__ = "fixture"
        with patch.dict(
            sys.modules,
            {"fixture": package, "fixture._enum": enums, "fixture.values": values},
        ):
            exec(  # noqa: S102 -- Exercise generated record conversions.
                files["python/maplibre_native_ffi/_generated_values.py"],
                values.__dict__,
            )
        return files, values

    def test_nested_copied_values_keep_absence_and_escape_keyword_fields(self):
        api = self.parse("""
typedef struct mln_position { double latitude; double longitude; } mln_position;
typedef struct mln_new_entry {
  mln_buffer_view title BIND("encoding=utf8;optional=empty");
  mln_position position;
  double class;
} mln_new_entry;
BIND("execution=query;result=mln_new_entry;shape=array;ownership=borrowed")
mln_status mln_map_new_entries(mln_map map, const mln_completion *completion, mln_diagnostic *out_diagnostic);
""")
        self.assertEqual(python.coverage(api)["generated"], ["mln_map_new_entries"])
        _, values = self.materialize(api)
        raw = {
            "title": None,
            "position": {"latitude": 3.25, "longitude": -8.5},
            "class_": 2.0,
        }
        copied = values.NewEntry._from_native(raw)
        raw["position"]["latitude"] = 0
        self.assertEqual(copied.position, values.Position(3.25, -8.5))
        self.assertIsNone(copied.title)
        self.assertEqual(copied.class_, 2.0)

    def test_input_identifiers_cannot_shadow_callback_runtime_locals(self):
        api = self.parse("""
BIND("execution=command;result=void;shape=none;ownership=value")
mln_status mln_map_match(mln_map map, double self, double input_self, double py,
                       mln_buffer_view title BIND("encoding=utf8"), double title_view,
                       const mln_completion *completion, mln_diagnostic *out_diagnostic);
""")
        files, _ = self.materialize(api)
        self.assertEqual(python.coverage(api)["generated"], ["mln_map_match"])
        facade = files["python/maplibre_native_ffi/_generated_operations.py"]
        self.assertIn(
            "def map_match(map: int, input_self_: float, input_self: float, input_py: float",
            facade,
        )
        native = files["src/generated_operations.rs"]
        self.assertIn(
            "sys::mln_map_match(map, input_self_, input_self, input_py, title_value, title_view, completion, diagnostic)",
            native,
        )

    def test_escaped_field_collisions_fail_before_generating_an_api(self):
        api = self.parse("""
typedef struct mln_entry { double class; double class_; } mln_entry;
BIND("execution=query;result=mln_entry;shape=value;ownership=borrowed")
mln_status mln_map_entry(mln_map map, const mln_completion *completion, mln_diagnostic *out_diagnostic);
""")
        result = python.coverage(api)
        self.assertEqual(result["generated"], [])
        self.assertIn("collide", result["unsupported"]["mln_map_entry"])

    def test_operations_cannot_take_owner_member_names(self):
        api = self.parse("""
typedef unsigned long long mln_host BIND("kind=handle;release=mln_host_destroy;parent=none");
BIND("execution=immediate") mln_status mln_host_destroy(mln_host host, mln_diagnostic *out_diagnostic);
BIND("execution=immediate") mln_status mln_host_closed(mln_host host, bool *out BIND("direction=out"), mln_diagnostic *out_diagnostic);
""")
        result = python.coverage(api)
        # The release alone becomes close.
        self.assertEqual(result["generated"], ["mln_host_destroy"])
        self.assertIn("owner member", result["unsupported"]["mln_host_closed"])

    def test_tagged_union_wrapper_keeps_payload_record_and_unknown_tag(self):
        api = self.parse(groups=("tagged_union",))
        _, values = self.materialize(api)
        event = values.Event._from_native(
            {"payload": {"kind": "frame", "value": {"timestamp": 12.5}}}
        )
        self.assertEqual(
            event.payload, values.EventFrameVariant(values.EventFrame(12.5))
        )
        unknown = values.Event._from_native({"payload": {"kind": None, "tag": 42}})
        self.assertEqual(unknown.payload, values.UnknownVariant(42))

    def test_decision_handle_owner_takes_its_name_from_the_issued_handle(self):
        api = self.parse(groups=("decision",))
        self.assertEqual(python.coverage(api)["unsupported"], {})
        files, values = self.materialize(api)
        native = files["src/generated_operations.rs"]
        self.assertIn('#[pyclass(name = "_TicketHandle")]', native)
        self.assertIn("module.add_class::<TicketHandle>()?;", native)
        self.assertIn("Py::new(py, TicketHandle {", native)
        # The core registration owns the cancel callback's native release.
        self.assertIn("self.state.on_cancel(Box::new(", native)
        self.assertIn(
            "_wrap_response(ticket, 'TicketHandle')",
            files["python/maplibre_native_ffi/_generated_values.py"],
        )
        self.assertEqual(
            values.TicketProvider.__annotations__["callback"],
            "Callable[[TicketHandle], Decision] | None",
        )
        self.assertIn(
            "class TicketHandle(_TicketHandleOperations, NativeHandleMixin):",
            files["python/maplibre_native_ffi/_generated_owners.py"],
        )
