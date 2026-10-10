"""Native emitter names preserve C declarations without shadowing runtime locals."""

import unittest

from support import parse

from tools.bindgen.emitters import go, swift, zig
from tools.bindgen.schema import validate


class NativeEmitterTests(unittest.TestCase):
    def parse(self, declarations):
        api = parse(declarations, defines=("MLN_PROTOCOL_MAP_RELEASE",))
        validate(api)
        return api

    def test_keyword_parameters_generate_for_commands(self):
        api = self.parse("""
BIND("execution=command")
mln_status mln_map_defer(mln_map map, double defer, double self, double raw,
                       double bindingArg0, const mln_completion *completion, mln_diagnostic *out_diagnostic);
""")
        for emitter in (go, swift, zig):
            with self.subTest(emitter=emitter.__name__):
                self.assertEqual(
                    emitter.coverage(api)["generated"],
                    ["mln_map_defer", "mln_map_release"],
                )

    def test_keyword_fields_generate_for_borrowed_arrays(self):
        api = self.parse("""
typedef struct mln_entry {
  mln_buffer_view type;
  mln_buffer_view defer;
} mln_entry;
BIND("execution=query;result=mln_entry;shape=array")
mln_status mln_map_entries(mln_map map, const mln_completion *completion, mln_diagnostic *out_diagnostic);
""")
        for emitter in (go, swift, zig):
            with self.subTest(emitter=emitter.__name__):
                self.assertEqual(
                    emitter.coverage(api)["generated"],
                    ["mln_map_entries", "mln_map_release"],
                )

    def test_runtime_method_collisions_have_explicit_coverage_failures(self):
        api = self.parse("""
BIND("execution=command")
mln_status mln_map_close(mln_map map, const mln_completion *completion, mln_diagnostic *out_diagnostic);
""")
        for emitter in (go, swift):
            with self.subTest(emitter=emitter.__name__):
                report = emitter.coverage(api)
                self.assertEqual(
                    report["generated"],
                    ["mln_map_release"],
                )
                self.assertIn("reserved", report["unsupported"]["mln_map_close"])
        self.assertEqual(
            zig.coverage(api)["generated"], ["mln_map_close", "mln_map_release"]
        )

    def test_nested_masked_values_follow_header_field_mutations(self):
        from tools.bindgen.emitters import rust

        header = """
typedef enum mln_probe_field { MLN_PROBE_POINT = 1, MLN_PROBE_GAIN = 2 } mln_probe_field;
typedef struct mln_probe_point { double x; double y; } mln_probe_point;
typedef struct mln_probe_options {
  unsigned fields BIND("enum=mln_probe_field");
  mln_probe_point point BIND("mask=fields;bit=MLN_PROBE_POINT");
  double gain BIND("mask=fields;bit=MLN_PROBE_GAIN");
} mln_probe_options;
BIND("execution=query;result=mln_probe_options")
mln_status mln_map_probe(mln_map map, const mln_completion *completion, mln_diagnostic *out_diagnostic);
BIND("execution=command")
mln_status mln_map_set_probe(mln_map map, const mln_probe_options *options, const mln_completion *completion, mln_diagnostic *out_diagnostic);
"""
        for emitter in (rust, swift, zig):
            sources = []
            for scalar in ("double", "_Bool"):
                api = self.parse(header.replace("double gain", scalar + " gain"))
                report = emitter.coverage(api)
                expected = {"mln_map_probe", "mln_map_set_probe"}
                expected.add("mln_map_release")
                self.assertEqual(
                    set(report["generated"]), expected, report["unsupported"]
                )
                output = emitter.generate(api)
                source = (
                    output if isinstance(output, str) else "\n".join(output.values())
                )
                sources.append(source)
            self.assertNotEqual(*sources)

    def test_zig_nested_retained_inputs_fail_closed(self):
        api = self.parse("""
typedef struct mln_store_options {
  mln_buffer_view view BIND("lifetime=owner");
} mln_store_options;
BIND("execution=command")
mln_status mln_map_store(mln_map map, const mln_store_options *options, const mln_completion *completion, mln_diagnostic *out_diagnostic);
""")
        report = zig.coverage(api)
        self.assertIn("retained input requires", report["unsupported"]["mln_map_store"])
        self.assertNotIn("mln_map_store", report["generated"])
