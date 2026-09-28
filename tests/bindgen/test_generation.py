"""Header mutations must drive emitter changes and explicit coverage failures."""

import unittest
from pathlib import Path
from tempfile import TemporaryDirectory

from tools.bindgen.emitters import dart, dotnet, go, kotlin, python, rust, swift, zig
from tools.bindgen.frontend import parse_headers
from tools.bindgen.model import ModelError
from tools.bindgen.schema import validate

EMITTERS = (dart, dotnet, go, kotlin, python, rust, swift, zig)

PRELUDE = """
#define BIND(x) __attribute__((annotate("mln:" x)))
typedef unsigned long long mln_map;
typedef int mln_status;
typedef struct mln_completion { void *state; } mln_completion;
typedef struct mln_buffer_view { const void *data; unsigned long size; } mln_buffer_view;
"""


class GenerationTests(unittest.TestCase):
    def parse(self, source, *, owned_map=False):
        with TemporaryDirectory() as directory:
            include = Path(directory)
            prelude = PRELUDE
            if owned_map:
                prelude = prelude.replace(
                    "typedef unsigned long long mln_map;",
                    'typedef unsigned long long mln_map BIND("kind=handle;release=mln_map_close;dispose=mln_map_close;parent=none");',
                )
                prelude += (
                    '\nBIND("execution=immediate") void mln_map_close(mln_map map);\n'
                )
            (include / "style.h").write_text(prelude + source)
            api = parse_headers(include)
        validate(api)
        return api

    def rendered(self, emitter, api):
        output = emitter.generate(api)
        return "\n".join(output.values()) if isinstance(output, dict) else output

    def test_portable_scalar_aliases_generate_identically_across_host_expansion(self):
        source = """
typedef unsigned UNDERLYING uint64_t;
typedef UNDERLYING int64_t;
typedef unsigned UNDERLYING size_t;
typedef uint64_t mln_counter;
typedef mln_counter mln_counter_alias;
typedef int64_t mln_offset;
typedef size_t mln_count;
typedef enum mln_flags : uint64_t { MLN_FLAGS_HIGH = 0x100000000ULL } mln_flags;
typedef struct mln_values { mln_counter_alias counter; mln_offset offset; mln_count count; mln_flags flags; } mln_values;
BIND("execution=immediate") mln_status mln_roundtrip(mln_values input, mln_values *out_value BIND("direction=out"));
"""
        before = self.parse(source.replace("UNDERLYING", "long"))
        after = self.parse(source.replace("UNDERLYING", "long long"))
        for emitter in EMITTERS:
            with self.subTest(emitter=emitter.__name__):
                self.assertEqual(emitter.coverage(before)["unsupported"], {})
                self.assertEqual(
                    self.rendered(emitter, before), self.rendered(emitter, after)
                )

    def test_added_and_renamed_function_generate_without_function_tables(self):
        for emitter in EMITTERS:
            with self.subTest(emitter=emitter.__name__):
                original = self.parse(
                    """
BIND("execution=query;result=double;shape=value;ownership=borrowed")
mln_status mln_map_test_scale(mln_map map, double latitude, const mln_completion *completion);
""",
                    owned_map=True,
                )
                renamed = self.parse(
                    """
BIND("execution=query;result=double;shape=value;ownership=borrowed")
mln_status mln_map_new_scale(mln_map map, float latitude, const mln_completion *completion);
BIND("receiver=map;execution=command;result=void;shape=none;ownership=value")
mln_status mln_map_new_command(mln_map map, bool enabled, const mln_completion *completion);
""",
                    owned_map=True,
                )
                before = self.rendered(emitter, original)
                after = self.rendered(emitter, renamed)
                self.assertIn("mln_map_test_scale", before)
                self.assertNotIn("mln_map_test_scale", after)
                self.assertIn("mln_map_new_command", after)
                self.assertIn("mln_map_new_scale", after)
                self.assertNotEqual(before, after)
                self.assertEqual(
                    set(emitter.coverage(renamed)["generated"]) - {"mln_map_close"},
                    {"mln_map_new_scale", "mln_map_new_command"},
                )

    def test_a_new_handle_generates_without_emitter_tables(self):
        api = self.parse("""
typedef unsigned long long mln_widget BIND("kind=handle;release=mln_widget_close;dispose=mln_widget_close;parent=none");
BIND("execution=immediate") void mln_widget_close(mln_widget widget);
BIND("execution=query;result=double;shape=value;ownership=borrowed")
mln_status mln_widget_scale(mln_widget widget, const mln_completion *completion);
""")
        for emitter in EMITTERS:
            with self.subTest(emitter=emitter.__name__):
                coverage = emitter.coverage(api)
                self.assertIn("mln_widget_scale", coverage["generated"])
                self.assertNotIn("mln_widget_scale", coverage["unsupported"])

    def test_partial_field_metadata_rejects_whole_record_and_operation(self):
        api = self.parse("""
typedef struct mln_new_entry {
  mln_buffer_view title BIND("encoding=utf8");
  const double *positions;
  unsigned long count;
} mln_new_entry;
BIND("execution=query;result=mln_new_entry;shape=array;ownership=borrowed")
mln_status mln_map_new_entries(mln_map map, const mln_completion *completion);
""")
        for emitter in EMITTERS:
            with self.subTest(emitter=emitter.__name__):
                coverage = emitter.coverage(api)
                self.assertEqual(coverage["generated"], [])
                self.assertTrue(coverage["unsupported"]["mln_map_new_entries"])
                self.assertNotIn("NewEntries(", self.rendered(emitter, api))

    def test_every_declaration_is_generated_or_has_a_reason(self):
        api = self.parse("""
BIND("execution=query;result=double;shape=value;ownership=borrowed")
mln_status mln_map_scalar(mln_map map, const mln_completion *completion);
BIND("execution=command;result=void;shape=none;ownership=value")
mln_status mln_map_span(mln_map map, const double *values, unsigned count, const mln_completion *completion);
BIND("execution=immediate")
void mln_global_hook(void (*callback)(void *), void *context);
""")
        names = set(api.functions_by_name)
        for emitter in EMITTERS:
            with self.subTest(emitter=emitter.__name__):
                coverage = emitter.coverage(api)
                generated = set(coverage["generated"])
                unsupported = set(coverage["unsupported"])
                self.assertFalse(generated & unsupported)
                self.assertEqual(generated | unsupported, names)
                self.assertIn("mln_map_scalar", generated)
                self.assertIn("values", coverage["unsupported"]["mln_map_span"])
                self.assertTrue(all(coverage["unsupported"].values()))

    def test_union_results_require_active_variant_decoding(self):
        api = self.parse("""
typedef union mln_variant { double number; bool flag; } mln_variant;
BIND("execution=query;result=mln_variant;shape=array;ownership=borrowed")
mln_status mln_map_variants(mln_map map, const mln_completion *completion);
""")
        for emitter in EMITTERS:
            with self.subTest(emitter=emitter.__name__):
                coverage = emitter.coverage(api)
                self.assertEqual(coverage["generated"], [])
                self.assertIn("mln_map_variants", coverage["unsupported"])

    def test_consumed_input_requires_a_resolved_owner(self):
        for contract, receiver in (
            (";consumes=success", "mln_map map"),
            ("", 'mln_map map BIND("consumes=always")'),
        ):
            api = self.parse(f"""
BIND("execution=command;result=void;shape=none;ownership=value{contract}")
mln_status mln_map_consume({receiver}, const mln_completion *completion);
""")
            for emitter in EMITTERS:
                with self.subTest(emitter=emitter.__name__, contract=contract):
                    coverage = emitter.coverage(api)
                    self.assertEqual(coverage["generated"], [])
                    self.assertIn("mln_map_consume", coverage["unsupported"])
                    self.assertNotIn("mln_map_consume", self.rendered(emitter, api))

    def test_consumption_policy_rejects_ambiguous_values(self):
        with self.assertRaisesRegex(ModelError, "unsupported consumes"):
            self.parse(
                'BIND("execution=immediate;consumes=map") void mln_map_consume(mln_map map);'
            )

    def test_nullable_record_field_preserves_pointer_presence(self):
        api = self.parse("""
typedef struct mln_nullable_entry {
  mln_buffer_view title BIND("encoding=utf8;nullable=true");
} mln_nullable_entry;
BIND("execution=query;result=mln_nullable_entry;shape=array;ownership=borrowed")
mln_status mln_map_nullable_entries(mln_map map, const mln_completion *completion);
""")
        for emitter in EMITTERS:
            with self.subTest(emitter=emitter.__name__):
                coverage = emitter.coverage(api)
                self.assertEqual(
                    coverage["generated"],
                    ["mln_map_nullable_entries"],
                    coverage["unsupported"],
                )
                self.assertIn("mln_map_nullable_entries", self.rendered(emitter, api))

    def test_scalar_bitfield_requires_a_layout_rule(self):
        api = self.parse("""
typedef unsigned int uint32_t;
typedef struct mln_bit_entry { uint32_t flags : 3; } mln_bit_entry;
BIND("execution=query;result=mln_bit_entry;shape=array;ownership=borrowed")
mln_status mln_map_bit_entries(mln_map map, const mln_completion *completion);
""")
        for emitter in EMITTERS:
            with self.subTest(emitter=emitter.__name__):
                coverage = emitter.coverage(api)
                self.assertEqual(coverage["generated"], [])
                self.assertTrue(coverage["unsupported"]["mln_map_bit_entries"])
                self.assertNotIn("mln_map_bit_entries", self.rendered(emitter, api))

    def test_retained_input_requires_a_lifetime_adapter(self):
        api = self.parse("""
BIND("execution=command;result=void;shape=none;ownership=value")
mln_status mln_map_store_view(mln_map map,
  mln_buffer_view view BIND("encoding=utf8;lifetime=owner"),
  const mln_completion *completion);
""")
        for emitter in EMITTERS:
            with self.subTest(emitter=emitter.__name__):
                coverage = emitter.coverage(api)
                self.assertEqual(coverage["generated"], [])
                self.assertIn("mln_map_store_view", coverage["unsupported"])


if __name__ == "__main__":
    unittest.main()
