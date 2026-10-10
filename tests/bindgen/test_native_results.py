"""Plan the native table of the result each completion function delivers."""

from __future__ import annotations

import unittest

from support import parse, real_api

from tools.bindgen.compiler import compile_api
from tools.bindgen.native_results import CompletionResultPlan, completion_results


class NativeResultTests(unittest.TestCase):
    def test_each_completion_records_its_value_type_and_shape(self):
        plans = completion_results(parse(groups=("completion_results",)))
        self.assertEqual(
            plans,
            (
                CompletionResultPlan("mln_map_find_tile", "mln_tile_id", False, True),
                CompletionResultPlan(
                    "mln_map_list_names", "mln_buffer_view", True, False
                ),
                CompletionResultPlan("mln_map_reload"),
                CompletionResultPlan(
                    "mln_map_visible_tiles", "mln_tile_id", True, True
                ),
                CompletionResultPlan("mln_map_zoom", "double", False, False),
            ),
        )

    def test_every_completion_but_a_command_has_an_entry(self):
        api = real_api()
        completions = {
            operation.name
            for operation in compile_api(api).operations
            if operation.completion is not None and operation.execution != "command"
        }
        annotated = {
            function.name
            for function in api.functions
            if function.metadata.get("result", "void") != "void"
        }
        plans = completion_results(api)
        self.assertEqual({plan.function for plan in plans}, completions)
        self.assertEqual(
            {plan.function for plan in plans if plan.type is not None}, annotated
        )


if __name__ == "__main__":
    unittest.main()
