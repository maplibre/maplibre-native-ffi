"""Compiler and contract tests with deliberately awkward C declarations."""

import unittest
from pathlib import Path
from tempfile import TemporaryDirectory

from support import parse, parse_sources, protocol_header

from tools.bindgen.frontend import parse_headers
from tools.bindgen.model import ModelError
from tools.bindgen.schema import validate


class FrontendTests(unittest.TestCase):
    def parse(self, source):
        return parse(source)

    def test_interface_entrypoints_separate_runtime_exports_and_reject_orphans(self):
        with TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "binding-interfaces.toml").write_text(
                'public = ["public.h"]\nruntime = ["runtime.h"]\n'
            )
            (root / "public.h").write_text(
                "#pragma once\n#define MLN_C_VERSION 0u\n"
                + protocol_header(
                    "mln_status public_operation(mln_diagnostic *out_diagnostic);\n"
                )
            )
            (root / "runtime.h").write_text(
                '#include "public.h"\ntypedef enum runtime_kind { RUNTIME_COPY = 1 } runtime_kind;\nmln_status runtime_operation(mln_diagnostic *out_diagnostic);\n'
            )
            api = parse_headers(root)
            self.assertEqual(api.runtime_exports, ("runtime_operation",))
            self.assertEqual(api.runtime_types, ("runtime_kind",))
            self.assertEqual(
                tuple(function.name for function in api.public_functions),
                ("public_operation",),
            )
            self.assertEqual(len(api.functions), 2)
            (root / "orphan.h").write_text(
                '#pragma once\n#include "public.h"\nmln_status omitted_operation(mln_diagnostic *out_diagnostic);\n'
            )
            with self.assertRaisesRegex(
                ModelError, "outside all interface entrypoints"
            ):
                parse_headers(root)
            with (root / "public.h").open("a") as header:
                header.write('#include "orphan.h"\n')
            api = parse_headers(root)
            self.assertEqual(
                tuple(function.name for function in api.public_functions),
                ("omitted_operation", "public_operation"),
            )
            self.assertEqual(api.runtime_exports, ("runtime_operation",))
            self.assertEqual(api.runtime_types, ("runtime_kind",))

    def test_reads_the_abi_version_macro_and_requires_it_of_the_real_headers(self):
        version = parse_sources({"api.h": "#define MLN_C_VERSION 7u\n"}).c_version
        self.assertEqual(version, 7)
        self.assertIsNone(parse_sources({"api.h": "#define OTHER 1\n"}).c_version)
        with self.assertRaisesRegex(ModelError, "MLN_C_VERSION must be an integer"):
            parse_sources({"api.h": "#define MLN_C_VERSION (1 + 1)\n"})
        with TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "binding-interfaces.toml").write_text(
                'public = ["public.h"]\nruntime = ["public.h"]\n'
            )
            (root / "public.h").write_text("#pragma once\n" + protocol_header(""))
            with self.assertRaisesRegex(ModelError, "no header defines MLN_C_VERSION"):
                parse_headers(root)

    def test_preserves_aliases_qualifiers_callbacks_and_nested_layout(self):
        api = self.parse("""
typedef void (*mln_callback)(void *context, const mln_map *maps BIND("length=count"), unsigned count);
typedef struct mln_value {
    unsigned flags;
    union { int integer; double number; } value;
    const mln_map *maps BIND("length=count");
    unsigned count;
    mln_callback callback;
    int fixed[3];
} mln_value;
mln_status mln_read(mln_runtime runtime, const mln_map *map,
                   double *out_value BIND("direction=out"), mln_diagnostic *out_diagnostic);
""")
        function = api.functions_by_name["mln_read"]
        self.assertEqual(function.parameters[0].type.declaration, "mln_runtime")
        self.assertEqual(function.parameters[1].type.pointee.declaration, "mln_map")
        self.assertTrue(function.parameters[1].type.pointee.const)
        # The frontend completes the annotation with the pointer's defaults.
        self.assertEqual(
            function.parameters[2].metadata,
            {"direction": "out", "lifetime": "call", "ownership": "borrowed"},
        )
        record = api.records_by_name["mln_value"]
        self.assertEqual(
            [field.name for field in record.fields],
            ["flags", "value", "maps", "count", "callback", "fixed"],
        )
        self.assertEqual(record.fields[-1].type.length, 3)
        callback = api.typedefs_by_name["mln_callback"].type.pointee
        self.assertEqual(callback.kind, "function")
        self.assertEqual(callback.parameters[1].pointee.declaration, "mln_map")
        callback_parameters = api.typedefs_by_name["mln_callback"].parameters
        self.assertEqual(callback_parameters[1].name, "maps")
        self.assertEqual(
            callback_parameters[1].metadata,
            {
                "direction": "in",
                "length": "count",
                "lifetime": "call",
                "ownership": "borrowed",
            },
        )
        validate(api)

    def test_enum_typedef_underlying_preserves_unsigned_high_bits(self):
        api = self.parse("""
typedef unsigned int sample_u32;
typedef unsigned long long sample_u64;
typedef signed int sample_i32;
typedef enum mln_u32 : sample_u32 { MLN_U32_HIGH = 0xf1234567U } mln_u32;
typedef enum mln_u64 : sample_u64 { MLN_U64_HIGH = 0xf123456789abcdefULL } mln_u64;
typedef enum mln_i32 : sample_i32 { MLN_I32_NEGATIVE = -17 } mln_i32;
""")
        values = {
            item.name: item.value
            for enum in api.enums
            if enum.name in {"mln_u32", "mln_u64", "mln_i32"}
            for item in enum.values
        }
        self.assertEqual(
            values,
            {
                "MLN_U32_HIGH": 0xF1234567,
                "MLN_U64_HIGH": 0xF123456789ABCDEF,
                "MLN_I32_NEGATIVE": -17,
            },
        )

    def test_model_is_independent_of_checkout_path(self):
        source = protocol_header("""
typedef struct mln_options { union { int one; float two; } choice; } mln_options;
mln_options mln_options_default(void);
""")
        models = []
        for _ in range(2):
            with TemporaryDirectory() as directory:
                (Path(directory) / "api.h").write_text(source)
                models.append(parse_headers(Path(directory)))
        self.assertEqual(*models)

    def test_query_requires_a_result_and_defaults_its_payload(self):
        with self.assertRaisesRegex(ModelError, "query requires a value payload"):
            validate(
                self.parse("""
BIND("execution=query")
mln_status mln_query(mln_map map, const mln_completion *completion, mln_diagnostic *out_diagnostic);
""")
            )
        api = self.parse("""
BIND("execution=query;result=double")
mln_status mln_query(mln_map map, const mln_completion *completion, mln_diagnostic *out_diagnostic);
""")
        validate(api)
        self.assertEqual(
            api.functions_by_name["mln_query"].metadata,
            {
                "execution": "query",
                "ownership": "borrowed",
                "result": "double",
                "shape": "value",
            },
        )

    def test_metadata_typos_and_const_output_are_rejected(self):
        api = self.parse("""
BIND("ownershp=borrowed")
mln_status mln_read(const double *value BIND("direction=out"), mln_diagnostic *out_diagnostic);
""")
        with self.assertRaises(ModelError) as raised:
            validate(api)
        self.assertIn("unknown metadata key 'ownershp'", str(raised.exception))
        self.assertIn("output direction contradicts const", str(raised.exception))

    def test_a_thread_start_wait_belongs_to_an_immediate_status_call(self):
        source = """
BIND("blocks=thread_start")
mln_status mln_start(mln_map *out_map BIND("direction=out"), mln_diagnostic *out_diagnostic);
"""
        validate(self.parse(source))
        for before, after in (
            (
                "mln_status mln_start(",
                'BIND("execution=snapshot") mln_status mln_start(',
            ),
            (
                'mln_status mln_start(mln_map *out_map BIND("direction=out"), mln_diagnostic *out_diagnostic);',
                "unsigned mln_start(void);",
            ),
        ):
            with (
                self.subTest(after=after),
                self.assertRaisesRegex(
                    ModelError,
                    "blocks requires an immediate function that returns a status",
                ),
            ):
                validate(self.parse(source.replace(before, after, 1)))
        with self.assertRaisesRegex(ModelError, "unsupported blocks='lock'"):
            self.parse(source.replace("thread_start", "lock"))

    def test_duplicate_metadata_is_rejected_at_the_declaration(self):
        with self.assertRaisesRegex(
            ModelError, "duplicate binding metadata 'execution'"
        ):
            self.parse("""
BIND("execution=immediate;execution=query") mln_status mln_duplicate(mln_diagnostic *out_diagnostic);
""")

    def test_bad_span_relation_and_scalar_encoding_are_rejected(self):
        api = self.parse("""
mln_status mln_bad_span(const double *items BIND("length=missing"),
                        double number BIND("encoding=json"), mln_diagnostic *out_diagnostic);
""")
        with self.assertRaises(ModelError) as raised:
            validate(api)
        self.assertIn("length names an absent parameter", str(raised.exception))
        self.assertIn("encoding requires a buffer view", str(raised.exception))

    def test_completion_requires_synchronous_acceptance_status(self):
        with self.assertRaisesRegex(ModelError, "mln_status acceptance gating"):
            validate(
                self.parse("""
BIND("execution=query;result=double")
void mln_broken_submission(mln_map map, const mln_completion *completion);
""")
            )

    def test_a_bitmask_enum_requires_a_flag_set_typedef(self):
        source = """
typedef enum BIND("kind=bitmask") mln_kind_flag : unsigned { MLN_KIND_FLAG_ONE = 1 } mln_kind_flag;
typedef unsigned mln_kind_flags;
typedef struct mln_record {
  mln_kind_flags kinds;
} mln_record;
"""
        validate(self.parse(source))
        for before, after in (
            ("typedef unsigned mln_kind_flags;", ""),
            (
                "typedef unsigned mln_kind_flags;",
                "typedef unsigned long long mln_kind_flags;",
            ),
        ):
            with (
                self.subTest(after=after),
                self.assertRaisesRegex(
                    ModelError,
                    "mln_kind_flag: a bitmask enum requires a mln_kind_flags "
                    "typedef of its underlying type",
                ),
            ):
                source_after = source.replace(before, after)
                if not after:
                    source_after = source_after.replace(
                        "mln_kind_flags kinds", "unsigned kinds"
                    )
                validate(self.parse(source_after))

    def test_status_functions_report_through_a_final_diagnostic(self):
        api = self.parse("""
mln_status mln_resize(mln_map map, double scale, mln_diagnostic *out_diagnostic);
""")
        (function,) = api.functions
        self.assertTrue(function.diagnostic)
        self.assertEqual(
            [parameter.name for parameter in function.parameters], ["map", "scale"]
        )
        validate(api)
        for declaration in (
            "mln_status mln_resize(mln_map map, double scale);",
            "void mln_resize(mln_map map, mln_diagnostic *out_diagnostic);",
        ):
            with (
                self.subTest(declaration=declaration),
                self.assertRaisesRegex(ModelError, "final mln_diagnostic"),
            ):
                validate(self.parse(f"{declaration}\n"))
        with self.assertRaisesRegex(ModelError, "must be the last parameter"):
            self.parse(
                "mln_status mln_resize(mln_diagnostic *out_diagnostic, mln_map map);\n"
            )


if __name__ == "__main__":
    unittest.main()
