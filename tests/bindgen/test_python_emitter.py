"""Exercise header-driven Python values and identifier isolation."""

import enum
import sys
import types
import unittest
from unittest.mock import MagicMock, patch

from support import parse

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

        modules = {
            "fixture": types.ModuleType("fixture"),
            "fixture._enum": module("_enum", UnknownIntEnum=enum.IntEnum),
            "fixture._native": native or MagicMock(),
            "fixture._completion": module("_completion", CommandCompletion=object),
            "fixture._future": module("_future", map_future=None),
            "fixture._operation": module(
                "_operation",
                GeneratedOperations=GeneratedOperations,
                _adopt_future=None,
                _adopt_value=None,
                _with_view=None,
            ),
            "fixture._lifecycle": module(
                "_lifecycle", NativeHandleMixin=NativeHandleMixin
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

    def test_nested_copied_values_keep_absence_and_escape_keyword_fields(self):
        api = self.parse("""
typedef struct mln_position { double latitude; double longitude; } mln_position;
typedef struct mln_new_entry {
  mln_buffer_view title BIND("optional=empty");
  mln_position position;
  double class;
} mln_new_entry;
BIND("execution=query;result=mln_new_entry;shape=array")
mln_status mln_map_new_entries(mln_map map, const mln_completion *completion, mln_diagnostic *out_diagnostic);
""")
        self.assertEqual(python.coverage(api)["generated"], ["mln_map_new_entries"])
        values = self.materialize(api).values
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

    def test_a_nested_registration_defaults_to_a_disabled_one(self):
        api = self.parse(groups=("record_notification",))
        self.assertIn("mln_map_request_reading", python.coverage(api)["generated"])
        values = self.materialize(api).values
        first = values.ReadingRequest(level=3)
        second = values.ReadingRequest(level=4)
        self.assertIsNone(first.handler.callback)
        self.assertIsNot(first.handler, second.handler)

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
mln_status mln_host_closed(mln_host host, bool *out BIND("direction=out"), mln_diagnostic *out_diagnostic);
""")
        result = python.coverage(api)
        # The release alone becomes close.
        self.assertEqual(result["generated"], ["mln_host_destroy"])
        self.assertIn("owner member", result["unsupported"]["mln_host_closed"])

    def test_tagged_union_wrapper_keeps_payload_record_and_unknown_tag(self):
        api = self.parse(groups=("tagged_union",))
        values = self.materialize(api).values
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
        generated = self.materialize(api)
        owners, values = generated.owners, generated.values
        self.assertEqual(
            values.TicketProvider.__annotations__["callback"],
            "Callable[[TicketHandle], Decision] | None",
        )
        # The provider hands its callback the issued handle's owner.
        values._wrap_response = lambda native, owner: (owner, native)
        received = []
        provider = values.TicketProvider(
            callback=lambda ticket: received.append(ticket) or values.Decision.CLAIM
        )
        self.assertEqual(provider._invoke_callback(41), values.Decision.CLAIM)
        self.assertEqual(received, [("TicketHandle", 41)])
        self.assertTrue(issubclass(owners.TicketHandle, generated.NativeHandleMixin))
        with self.assertRaises(TypeError):
            owners.TicketHandle()
        native = MagicMock()
        ticket = owners.TicketHandle._from_native(native)
        ticket.on_cancel(print)
        native.on_cancel.assert_called_once_with(print)
