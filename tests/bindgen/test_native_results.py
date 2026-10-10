"""Plan the native table of the value each completion function delivers."""

from __future__ import annotations

import unittest

from support import parse, real_api

from tools.bindgen.native_results import CompletionResultPlan, completion_results


class NativeResultTests(unittest.TestCase):
    def test_each_value_completion_records_its_type_and_shape(self):
        plans = completion_results(parse(groups=("completion_results",)))
        self.assertEqual(
            plans,
            (
                CompletionResultPlan("mln_map_find_tile", "mln_tile_id", False, True),
                CompletionResultPlan(
                    "mln_map_list_names", "mln_buffer_view", True, False
                ),
                CompletionResultPlan(
                    "mln_map_visible_tiles", "mln_tile_id", True, True
                ),
                CompletionResultPlan("mln_map_zoom", "double", False, False),
            ),
        )

    def test_every_annotated_result_has_an_entry(self):
        api = real_api()
        annotated = {
            function.name
            for function in api.functions
            if function.metadata.get("result", "void") != "void"
        }
        self.assertEqual({plan.function for plan in completion_results(api)}, annotated)


if __name__ == "__main__":
    unittest.main()
