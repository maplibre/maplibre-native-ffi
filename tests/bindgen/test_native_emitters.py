"""Native emitter names preserve C declarations without shadowing runtime locals."""

import unittest
from pathlib import Path
from tempfile import TemporaryDirectory

from tools.bindgen.emitters import go, swift, zig
from tools.bindgen.frontend import parse_headers
from tools.bindgen.schema import validate


class NativeEmitterTests(unittest.TestCase):
    def parse(self, declarations):
        with TemporaryDirectory() as directory:
            headers = Path(directory)
            (headers / "api.h").write_text(
                """
#define BIND(x) __attribute__((annotate("mln:" x)))
typedef unsigned long long mln_map BIND("kind=handle;release=mln_map_release;parent=none");
BIND("execution=immediate") void mln_map_release(mln_map map BIND("consumes=always"));
typedef int mln_status;
typedef struct mln_diagnostic { unsigned int size; char message[4096]; } mln_diagnostic;
typedef struct mln_completion { void *state; } mln_completion;
typedef struct mln_buffer_view { const void *data; unsigned long size; } mln_buffer_view;
"""
                + declarations
            )
            api = parse_headers(headers)
        validate(api)
        return api

    def test_keywords_and_runtime_parameter_names_use_separate_local_names(self):
        api = self.parse("""
BIND("execution=command;result=void;shape=none;ownership=value")
mln_status mln_map_defer(mln_map map, double defer, double self, double raw,
                       double bindingArg0, const mln_completion *completion, mln_diagnostic *out_diagnostic);
""")
        for emitter in (go, swift, zig):
            with self.subTest(emitter=emitter.__name__):
                self.assertEqual(
                    emitter.coverage(api)["generated"],
                    ["mln_map_defer", "mln_map_release"],
                )
        go_source = go.generate(api)["generated_api.go"]
        self.assertIn(
            "Defer(defer_ float64, self float64, raw float64, bindingArg0 float64)",
            go_source,
        )
        swift_source = "\n".join(swift.generate(api).values())
        self.assertIn(
            "func `defer`(`defer` bindingArg0: Double, `self` bindingArg1: Double, `raw` bindingArg2: Double, `bindingArg0` bindingArg3: Double)",
            swift_source,
        )
        zig_source = zig.generate(api)["generated_api.zig"]
        self.assertIn(
            '@"map": Map, @"defer_input": f64, @"self": f64, @"raw_input": f64, @"bindingArg0": f64',
            zig_source,
        )
        self.assertIn("pub fn mapDefer(", zig_source)

    def test_keyword_fields_preserve_native_spelling_and_public_escape(self):
        api = self.parse("""
typedef struct mln_entry {
  mln_buffer_view type BIND("encoding=utf8");
  mln_buffer_view defer BIND("encoding=utf8");
} mln_entry;
BIND("execution=query;result=mln_entry;shape=array;ownership=borrowed")
mln_status mln_map_entries(mln_map map, const mln_completion *completion, mln_diagnostic *out_diagnostic);
""")
        self.assertIn("raw._type", go.generate(api)["generated_api.go"])
        self.assertIn("raw._defer", go.generate(api)["generated_api.go"])
        self.assertIn("raw.`defer`", "\n".join(swift.generate(api).values()))
        self.assertIn('raw.@"defer"', zig.generate(api)["generated_api.zig"])

    def test_runtime_method_collisions_have_explicit_coverage_failures(self):
        api = self.parse("""
BIND("execution=command;result=void;shape=none;ownership=value")
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
        self.assertIn("pub fn mapClose(", zig.generate(api)["generated_api.zig"])

    def test_nested_masked_values_follow_header_field_mutations(self):
        from tools.bindgen.emitters import rust

        header = """
typedef enum mln_probe_field { MLN_PROBE_POINT = 1, MLN_PROBE_GAIN = 2 } mln_probe_field;
typedef struct mln_probe_point { double x; double y; } mln_probe_point;
typedef struct mln_probe_options {
  unsigned fields BIND("kind=presence_mask;enum=mln_probe_field");
  mln_probe_point point BIND("mask=fields;bit=MLN_PROBE_POINT");
  double gain BIND("mask=fields;bit=MLN_PROBE_GAIN");
} mln_probe_options;
BIND("execution=query;result=mln_probe_options;shape=value;ownership=borrowed")
mln_status mln_map_probe(mln_map map, const mln_completion *completion, mln_diagnostic *out_diagnostic);
BIND("execution=command;result=void;shape=none;ownership=value")
mln_status mln_map_set_probe(mln_map map, const mln_probe_options *options BIND("length=1"), const mln_completion *completion, mln_diagnostic *out_diagnostic);
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
                self.assertIn("MLN_PROBE_POINT", source)
                self.assertIn("MLN_PROBE_GAIN", source)
                self.assertIn("ProbePoint", source)
                sources.append(source)
            self.assertNotEqual(*sources)

    def test_zig_nested_retained_inputs_fail_closed(self):
        api = self.parse("""
typedef struct mln_store_options {
  mln_buffer_view view BIND("encoding=utf8;lifetime=owner");
} mln_store_options;
BIND("execution=command;result=void;shape=none;ownership=value")
mln_status mln_map_store(mln_map map, const mln_store_options *options BIND("length=1"), const mln_completion *completion, mln_diagnostic *out_diagnostic);
""")
        report = zig.coverage(api)
        self.assertIn("retained input requires", report["unsupported"]["mln_map_store"])
        self.assertNotIn("mln_map_store", report["generated"])
