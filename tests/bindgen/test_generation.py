"""Header mutations must drive emitter changes and explicit coverage failures."""

import re
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
    typescript,
    zig,
)
from tools.bindgen.model import ModelError
from tools.bindgen.schema import validate

EMITTERS = (dart, dotnet, go, kotlin, python, rust, swift, typescript, zig)

# Protocol shapes an emitter's runtime cannot represent yet. Each emitter
# reports these declarations as unsupported with its reason.
PROTOCOL_GAPS = {
    ("values", "rust"): {"mln_probe_roundtrip"},
    ("absent_value", "dotnet"): {"mln_probe_read_level"},
    ("absent_value", "kotlin"): {"mln_probe_read_level"},
    ("absent_value", "python"): {"mln_probe_read_level"},
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

    def test_caller_sized_outputs_share_the_input_array_count(self):
        api = parse(groups=("caller_sized_output",))
        validate(api)
        bound = compile_api(api)
        for plan in bound.operations:
            output = plan.outputs[0]
            self.assertEqual(output.value.kind, "array")
            self.assertEqual(output.value.length, "point_count")
            self.assertEqual(output.sized_by, "points")
        for emitter in EMITTERS:
            with self.subTest(emitter=emitter.__name__):
                self.assertEqual(emitter.coverage(bound)["unsupported"], {})
                self.rendered(emitter, bound)

    def test_caller_sized_text_hides_its_capacity_and_length(self):
        api = parse(groups=("caller_sized_text",))
        validate(api)
        bound = compile_api(api)
        (plan,) = bound.operations
        text = plan.caller_sized_text
        self.assertEqual(text.name, "out_json")
        self.assertEqual(text.result_length, "out_json_length")
        self.assertEqual(text.value.kind, "buffer")
        self.assertEqual(text.value.length, "json_capacity")
        self.assertEqual(text.value.encoding, "json")
        for emitter in EMITTERS:
            with self.subTest(emitter=emitter.__name__):
                self.assertEqual(emitter.coverage(bound)["unsupported"], {})
                self.rendered(emitter, bound)

    def test_caller_sized_text_rejects_an_unsound_contract(self):
        text = 'char *out_json BIND("direction=out;encoding=json;length=capacity;result_length=out_length")'
        length = 'unsigned long *out_length BIND("direction=out")'
        cases = {
            "mutable character output": f'const char *out_json BIND("encoding=json;length=capacity;result_length=out_length"), unsigned long capacity, {length}',
            "encoding=json": f'char *out_json BIND("direction=out;encoding=bytes;length=capacity;result_length=out_length"), unsigned long capacity, {length}',
            "unsigned capacity input": f"{text}, int capacity, {length}",
            "output of the capacity's type": f'{text}, unsigned long capacity, unsigned *out_length BIND("direction=out")',
            "follow every input": f"{text}, unsigned long capacity, {length}, unsigned later",
            "only output": f'{text}, unsigned long capacity, {length}, unsigned *out_count BIND("direction=out")',
        }
        for error, parameters in cases.items():
            with (
                self.subTest(error=error),
                self.assertRaisesRegex(ModelError, re.escape(error)),
            ):
                self.parse(
                    f"mln_status mln_probe_text({parameters}, mln_diagnostic *out_diagnostic);"
                )

    def test_output_array_requires_a_counted_input_array(self):
        with self.assertRaisesRegex(ModelError, "output array length"):
            self.parse("""
mln_status mln_probe_batch(unsigned point_count, double *out_values BIND("direction=out;length=point_count"), mln_diagnostic *out_diagnostic);
""")

    def test_output_array_cannot_share_its_count_with_two_input_arrays(self):
        with self.assertRaisesRegex(ModelError, "output array length"):
            self.parse("""
mln_status mln_probe_batch(const double *left BIND("length=count"), const double *right BIND("length=count"), unsigned count,
  double *out_values BIND("direction=out;length=count"), mln_diagnostic *out_diagnostic);
""")

    def test_output_array_follows_every_input(self):
        for later in ("unsigned count", "unsigned flag"):
            counted = "" if later == "unsigned count" else "unsigned count, "
            with (
                self.subTest(later=later),
                self.assertRaisesRegex(ModelError, "follow every input"),
            ):
                self.parse(f"""
mln_status mln_probe_batch(const double *values BIND("length=count"), {counted}
  double *out_values BIND("direction=out;length=count"), {later}, mln_diagnostic *out_diagnostic);
""")

    def test_output_array_cannot_take_its_count_from_a_byte_buffer(self):
        with self.assertRaisesRegex(ModelError, "output array length"):
            self.parse("""
mln_status mln_probe_batch(const void *bytes BIND("length=count;encoding=bytes"), unsigned count,
  double *out_values BIND("direction=out;length=count"), mln_diagnostic *out_diagnostic);
""")

    def test_every_protocol_shape_generates_or_reports_its_gap(self):
        for group in protocol_groups():
            api = parse(groups=(group,))
            validate(api)
            bound = compile_api(api)
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
                    # functions back the generated values that call them, and
                    # a generated drain releases its own batch.
                    released = {
                        bound.handles[plan.drain.batch.value.unwrapped.native].release
                        for plan in bound.operations
                        if plan.drain and plan.name in coverage["generated"]
                    }
                    self.assertEqual(
                        set(coverage["generated"])
                        | set(coverage["unsupported"])
                        | set(coverage.get("support", ()))
                        | released,
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
typedef struct mln_values { mln_counter_alias counter; mln_offset offset; mln_count count; uint64_t flags BIND("enum=mln_flags"); } mln_values;
mln_status mln_roundtrip(mln_values input, mln_values *out_value BIND("direction=out"), mln_diagnostic *out_diagnostic);
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

    def test_every_declaration_is_generated_or_has_a_reason(self):
        api = self.parse("""
BIND("execution=query;result=double")
mln_status mln_map_scalar(mln_map map, const mln_completion *completion, mln_diagnostic *out_diagnostic);
BIND("execution=command")
mln_status mln_map_span(mln_map map, const double *values, unsigned count, const mln_completion *completion, mln_diagnostic *out_diagnostic);
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


if __name__ == "__main__":
    unittest.main()
