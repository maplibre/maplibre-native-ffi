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
mln_status mln_map_new_entries(mln_map map, const mln_completion *completion);
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
                       const mln_completion *completion);
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
            "sys::mln_map_match(map, input_self_, input_self, input_py, title_value, title_view, completion)",
            native,
        )

    def test_escaped_field_collisions_fail_before_generating_an_api(self):
        api = self.parse("""
typedef struct mln_entry { double class; double class_; } mln_entry;
BIND("execution=query;result=mln_entry;shape=value;ownership=borrowed")
mln_status mln_map_entry(mln_map map, const mln_completion *completion);
""")
        result = python.coverage(api)
        self.assertEqual(result["generated"], [])
        self.assertIn("collide", result["unsupported"]["mln_map_entry"])

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
mln_status mln_map_event(mln_map map, const mln_completion *completion);
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
