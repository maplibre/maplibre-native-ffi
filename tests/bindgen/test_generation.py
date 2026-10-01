"""Header mutations must drive emitter changes and explicit coverage failures."""

import unittest

from support import parse, protocol_groups

from tools.bindgen.compiler import compile_api
from tools.bindgen.emitters import (
    dart,
    dart_native,
    dotnet,
    go,
    kotlin,
    python,
    rust,
    swift,
    zig,
)
from tools.bindgen.model import ModelError
from tools.bindgen.schema import validate

EMITTERS = (dart, dotnet, go, kotlin, python, rust, swift, zig)

# Protocol shapes an emitter's runtime cannot represent yet. Each emitter
# reports these declarations as unsupported with its reason.
PROTOCOL_GAPS = {
    ("values", "dart"): {"mln_probe_nullable_text"},
    ("values", "kotlin"): {"mln_probe_nullable_text"},
    ("values", "rust"): {"mln_probe_nullable_text", "mln_probe_roundtrip"},
    ("owned_output", "dotnet"): {"mln_seed_plant"},
    ("direct_registration", "python"): {"mln_ticket_on_cancel"},
    ("direct_registration", "rust"): {"mln_ticket_on_cancel"},
    ("decision", "dart"): {"mln_host_set_provider"},
}


class GenerationTests(unittest.TestCase):
    def parse(self, source, *, owned_map=False):
        api = parse(
            source,
            defines=("MLN_PROTOCOL_MAP_CLOSE",) if owned_map else (),
            header="style.h",
        )
        validate(api)
        return api

    def test_every_protocol_shape_generates_or_reports_its_gap(self):
        for group in protocol_groups():
            api = parse(groups=(group,))
            validate(api)
            for emitter in EMITTERS:
                language = emitter.__name__.rsplit(".", 1)[-1]
                with self.subTest(group=group, emitter=language):
                    coverage = emitter.coverage(api)
                    self.assertEqual(
                        set(coverage["unsupported"]),
                        PROTOCOL_GAPS.get((group, language), set()),
                        coverage["unsupported"],
                    )
                    # No declaration disappears without a reason. Support
                    # functions back the generated values that call them.
                    self.assertEqual(
                        set(coverage["generated"])
                        | set(coverage["unsupported"])
                        | set(coverage.get("support", ())),
                        {function.name for function in api.public_functions},
                    )

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
BIND("execution=immediate") mln_status mln_roundtrip(mln_values input, mln_values *out_value BIND("direction=out"), mln_diagnostic *out_diagnostic);
"""
        before = self.parse(source.replace("UNDERLYING", "long"))
        after = self.parse(source.replace("UNDERLYING", "long long"))
        for emitter in EMITTERS:
            with self.subTest(emitter=emitter.__name__):
                self.assertEqual(emitter.coverage(before)["unsupported"], {})
                self.assertEqual(
                    self.rendered(emitter, before), self.rendered(emitter, after)
                )
        # Dart's declarations come from the bound model rather than the API.
        self.assertEqual(
            dart_native.generate(compile_api(before)),
            dart_native.generate(compile_api(after)),
        )

    def test_added_and_renamed_function_generate_without_function_tables(self):
        for emitter in EMITTERS:
            with self.subTest(emitter=emitter.__name__):
                original = self.parse(
                    """
BIND("execution=query;result=double;shape=value;ownership=borrowed")
mln_status mln_map_test_scale(mln_map map, double latitude, const mln_completion *completion, mln_diagnostic *out_diagnostic);
""",
                    owned_map=True,
                )
                renamed = self.parse(
                    """
BIND("execution=query;result=double;shape=value;ownership=borrowed")
mln_status mln_map_new_scale(mln_map map, float latitude, const mln_completion *completion, mln_diagnostic *out_diagnostic);
BIND("receiver=map;execution=command;result=void;shape=none;ownership=value")
mln_status mln_map_new_command(mln_map map, bool enabled, const mln_completion *completion, mln_diagnostic *out_diagnostic);
""",
                    owned_map=True,
                )
                self.assertEqual(
                    set(emitter.coverage(original)["generated"]) - {"mln_map_close"},
                    {"mln_map_test_scale"},
                )
                self.assertEqual(
                    set(emitter.coverage(renamed)["generated"]) - {"mln_map_close"},
                    {"mln_map_new_scale", "mln_map_new_command"},
                )

    def test_a_new_handle_generates_without_emitter_tables(self):
        api = self.parse("""
typedef unsigned long long mln_widget BIND("kind=handle;release=mln_widget_close;dispose=mln_widget_close;parent=none");
BIND("execution=immediate") void mln_widget_close(mln_widget widget);
BIND("execution=query;result=double;shape=value;ownership=borrowed")
mln_status mln_widget_scale(mln_widget widget, const mln_completion *completion, mln_diagnostic *out_diagnostic);
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
mln_status mln_map_new_entries(mln_map map, const mln_completion *completion, mln_diagnostic *out_diagnostic);
""")
        for emitter in EMITTERS:
            with self.subTest(emitter=emitter.__name__):
                coverage = emitter.coverage(api)
                self.assertEqual(coverage["generated"], [])
                self.assertTrue(coverage["unsupported"]["mln_map_new_entries"])

    def test_every_declaration_is_generated_or_has_a_reason(self):
        api = self.parse("""
BIND("execution=query;result=double;shape=value;ownership=borrowed")
mln_status mln_map_scalar(mln_map map, const mln_completion *completion, mln_diagnostic *out_diagnostic);
BIND("execution=command;result=void;shape=none;ownership=value")
mln_status mln_map_span(mln_map map, const double *values, unsigned count, const mln_completion *completion, mln_diagnostic *out_diagnostic);
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
mln_status mln_map_variants(mln_map map, const mln_completion *completion, mln_diagnostic *out_diagnostic);
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
mln_status mln_map_consume({receiver}, const mln_completion *completion, mln_diagnostic *out_diagnostic);
""")
            for emitter in EMITTERS:
                with self.subTest(emitter=emitter.__name__, contract=contract):
                    coverage = emitter.coverage(api)
                    self.assertEqual(coverage["generated"], [])
                    self.assertIn("mln_map_consume", coverage["unsupported"])

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
mln_status mln_map_nullable_entries(mln_map map, const mln_completion *completion, mln_diagnostic *out_diagnostic);
""")
        for emitter in EMITTERS:
            with self.subTest(emitter=emitter.__name__):
                coverage = emitter.coverage(api)
                self.assertEqual(
                    coverage["generated"],
                    ["mln_map_nullable_entries"],
                    coverage["unsupported"],
                )

    def test_scalar_bitfield_requires_a_layout_rule(self):
        api = self.parse("""
typedef unsigned int uint32_t;
typedef struct mln_bit_entry { uint32_t flags : 3; } mln_bit_entry;
BIND("execution=query;result=mln_bit_entry;shape=array;ownership=borrowed")
mln_status mln_map_bit_entries(mln_map map, const mln_completion *completion, mln_diagnostic *out_diagnostic);
""")
        for emitter in EMITTERS:
            with self.subTest(emitter=emitter.__name__):
                coverage = emitter.coverage(api)
                self.assertEqual(coverage["generated"], [])
                self.assertTrue(coverage["unsupported"]["mln_map_bit_entries"])

    def test_retained_input_requires_a_lifetime_adapter(self):
        api = self.parse("""
BIND("execution=command;result=void;shape=none;ownership=value")
mln_status mln_map_store_view(mln_map map,
  mln_buffer_view view BIND("encoding=utf8;lifetime=owner"),
  const mln_completion *completion, mln_diagnostic *out_diagnostic);
""")
        for emitter in EMITTERS:
            with self.subTest(emitter=emitter.__name__):
                coverage = emitter.coverage(api)
                self.assertEqual(coverage["generated"], [])
                self.assertIn("mln_map_store_view", coverage["unsupported"])


if __name__ == "__main__":
    unittest.main()
