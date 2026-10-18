"""The devcontainer aggregate must keep wrapped TOML tool assignments intact."""

from __future__ import annotations

import unittest

from devcontainer_tools import parse_tool_assignments


class ParseToolAssignmentsTest(unittest.TestCase):
    def test_keeps_wrapped_inline_tables_as_one_assignment(self) -> None:
        body = (
            '"aqua:facebook/ktfmt" = "{{vars.ktfmt_version}}"\n'
            '"core:zig" = { version = "{{vars.zig_version}}", os = [\n'
            '  "linux",\n'
            '  "macos",\n'
            '  "windows/x64",\n'
            "] }\n"
            '"core:go" = "{{vars.go_version}}"\n'
        )
        tools = parse_tool_assignments(body)
        self.assertEqual(
            set(tools),
            {"aqua:facebook/ktfmt", "core:zig", "core:go"},
        )
        self.assertIn("windows/x64", tools["core:zig"])
        self.assertTrue(tools["core:zig"].startswith('"core:zig"'))
        self.assertNotIn("]", tools)
        self.assertNotIn("linux", tools)
