"""Native emitters report a declaration they cannot generate, with the reason."""

import unittest

from support import parse

from tools.bindgen.emitters import go, swift, zig
from tools.bindgen.schema import validate


class NativeEmitterTests(unittest.TestCase):
    def parse(self, declarations):
        api = parse(declarations, defines=("MLN_PROTOCOL_MAP_RELEASE",))
        validate(api)
        return api

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
