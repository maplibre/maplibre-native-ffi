"""Exercise header-driven Python values and identifier isolation."""

import enum
import sys
import types
import unittest
from pathlib import Path
from tempfile import TemporaryDirectory
from unittest.mock import patch

from tools.bindgen.emitters import python
from tools.bindgen.frontend import parse_headers
from tools.bindgen.schema import validate

PRELUDE = """
#define BIND(x) __attribute__((annotate("mln:" x)))
typedef unsigned long long mln_map;
typedef int mln_status;
typedef struct mln_diagnostic { unsigned int size; char message[4096]; } mln_diagnostic;
typedef struct mln_completion { void *state; } mln_completion;
typedef struct mln_buffer_view { const void *data; unsigned long size; } mln_buffer_view;
"""


class PythonEmitterTests(unittest.TestCase):
    def parse(self, source):
        with TemporaryDirectory() as directory:
            headers = Path(directory)
            (headers / "api.h").write_text(PRELUDE + source)
            api = parse_headers(headers)
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
        api = self.parse("""
typedef enum mln_event_kind { MLN_EVENT_NONE = 0, MLN_EVENT_FRAME = 1 } mln_event_kind;
typedef struct mln_event_frame { double timestamp; } mln_event_frame;
typedef union mln_event_payload {
  mln_event_frame frame BIND("variant=MLN_EVENT_FRAME");
} mln_event_payload;
typedef struct mln_event {
  unsigned int kind BIND("kind=tag;enum=mln_event_kind");
  mln_event_payload payload BIND("tag=kind");
} mln_event;
BIND("execution=query;result=mln_event;shape=value;ownership=borrowed")
mln_status mln_map_event(mln_map map, const mln_completion *completion, mln_diagnostic *out_diagnostic);
""")
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
        api = self.parse("""
typedef enum decision : unsigned { DELEGATE = 0, CLAIM = 1 } decision;
typedef unsigned long long mln_host BIND("kind=handle;release=mln_host_destroy;parent=none");
typedef unsigned long long mln_ticket BIND("kind=handle;release=mln_ticket_release;parent=none");
typedef void (*release_context)(void *context BIND("kind=context;lifetime=owner")) BIND("thread=native;failure=contain");
typedef void (*cancel)(void *context BIND("kind=context;lifetime=owner")) BIND("thread=native;failure=contain");
typedef unsigned (*provider)(void *context BIND("kind=context;lifetime=owner"), mln_ticket ticket) BIND("thread=native;enum=decision;failure=DELEGATE;decision_handle=ticket;decision_accept=CLAIM;decision_pass=DELEGATE;complete=mln_ticket_answer;cancelled=mln_ticket_cancelled;cancel_registration=mln_ticket_on_cancel;wait_retired=mln_ticket_await");
typedef struct mln_ticket_provider {
  provider callback;
  void *user_data BIND("kind=context;ownership=borrowed");
  release_context release;
} mln_ticket_provider BIND("kind=callback_registration;user_data=user_data;release=release");
BIND("execution=immediate") mln_status mln_host_destroy(mln_host host, mln_diagnostic *out_diagnostic);
BIND("execution=command;result=void;shape=none;ownership=value")
mln_status mln_host_set_provider(mln_host host, const mln_ticket_provider *provider BIND("length=1"), const mln_completion *completion, mln_diagnostic *out_diagnostic);
BIND("execution=immediate") mln_status mln_ticket_answer(mln_ticket ticket, unsigned response, mln_diagnostic *out_diagnostic);
BIND("execution=immediate") mln_status mln_ticket_cancelled(mln_ticket ticket, bool *result BIND("direction=out"), mln_diagnostic *out_diagnostic);
BIND("execution=immediate;registration=callback;user_data=context;release_callback=release;accepted_unless=cancelled") mln_status mln_ticket_on_cancel(
  mln_ticket ticket, cancel callback, void *context BIND("kind=context"), release_context release, bool *cancelled BIND("direction=out"), mln_diagnostic *out_diagnostic);
BIND("execution=immediate") void mln_ticket_release(mln_ticket ticket);
BIND("execution=immediate") mln_status mln_ticket_await(mln_ticket ticket BIND("handle_access=issued"), mln_diagnostic *out_diagnostic);
""")
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
