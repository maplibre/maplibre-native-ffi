"""Header mutations must drive emitter changes and explicit coverage failures."""

import unittest

from support import parse, protocol_groups, real_api

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
    def test_go_guards_only_owner_outputs_copied_after_native_returns(self):
        from tools.bindgen.emitters.go_operations import needs_read

        api = compile_api(real_api())
        guarded = {plan.name for plan in api.operations if needs_read(plan)}
        self.assertEqual(guarded, {"mln_render_frame_batch_get"})

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
