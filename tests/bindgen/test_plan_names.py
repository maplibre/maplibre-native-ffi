"""Names and relations that the semantic plan derives once for every binding."""

import unittest

from support import parse

from tools.bindgen.compiler import compile_api
from tools.bindgen.model import ModelError
from tools.bindgen.schema import validate
from tools.bindgen.semantic import bind


class PlanNameTests(unittest.TestCase):
    def bind(self, source=""):
        api = parse(source, groups=("plan_names",))
        validate(api)
        return compile_api(api)

    def test_members_strip_the_declared_handle_prefix(self):
        bound = self.bind()
        operations = bound.operations_by_name
        self.assertEqual(bound.handles["mln_pass_handle"].stem, "pass")
        self.assertEqual(
            {
                name: operations[name].member
                for name in ("mln_pass_redeem", "mln_pass_stamp", "mln_pass_close")
            },
            {
                "mln_pass_redeem": "redeem",
                "mln_pass_stamp": "stamp",
                "mln_pass_close": "close",
            },
        )
        self.assertEqual(
            (operations["mln_pass_redeem"].status, operations["mln_pass_close"].status),
            (True, False),
        )

    def test_mask_flag_members_come_from_the_bit_enum(self):
        window = self.bind().values["mln_frame_window"]
        self.assertEqual([flag.member for flag in window.mask_flags], ["locked"])

    def test_ordered_fields_are_declared_on_a_record_of_plain_values(self):
        bound = self.bind()
        self.assertTrue(bound.values["mln_point"].ordered)
        self.assertFalse(bound.values["mln_frame_window"].ordered)
        with self.assertRaisesRegex(
            ModelError, "fields=ordered requires a struct of plain values"
        ):
            self.bind(
                'typedef struct mln_span { const char *text; } mln_span BIND("fields=ordered");\n'
                "mln_status mln_pass_span(mln_pass_handle pass, mln_span span, mln_diagnostic *out_diagnostic);\n"
            )

    def test_a_prefix_names_handle_operations_only(self):
        with self.assertRaisesRegex(ModelError, "prefix names a handle's operations"):
            self.bind(
                'typedef struct mln_box { double side; } mln_box BIND("prefix=mln_cube");\n'
            )

    def test_only_a_leading_handle_is_the_receiver(self):
        operation = self.bind(
            "mln_status mln_pass_after(int count, mln_pass_handle pass, mln_diagnostic *out_diagnostic);\n"
        ).operations_by_name["mln_pass_after"]
        self.assertEqual((operation.receiver, operation.member), (None, "pass_after"))

    def test_a_marshalled_input_lasts_for_the_call(self):
        source = "mln_status mln_pass_note(mln_pass_handle pass, const char *text{}, mln_diagnostic *out_diagnostic);\n"
        bind(parse(source.format(""), groups=("plan_names",)), require_complete=True)
        with self.assertRaisesRegex(
            ModelError, "retained input requires a lifetime contract"
        ):
            bind(
                parse(source.format(' BIND("lifetime=owner")'), groups=("plan_names",)),
                require_complete=True,
            )


if __name__ == "__main__":
    unittest.main()
