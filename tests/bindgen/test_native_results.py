"""Plan the native table of the result each completion function delivers."""

from __future__ import annotations

import unittest

from support import parse

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


if __name__ == "__main__":
    unittest.main()
