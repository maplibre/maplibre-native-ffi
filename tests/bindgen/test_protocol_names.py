"""The generator names C declarations only in protocol.py and policy tables."""

import ast
import unittest

from support import ROOT, real_api

GENERATOR = ROOT / "tools/bindgen"

# Per-binding policy tables keyed by C name, by file and assignment target.
POLICY_TABLES = {
    "emitters/dart.py": {"HOST_ONLY"},
    "emitters/dart_values.py": {"NOTES"},
    "emitters/dotnet.py": {"NOTES"},
    "emitters/kotlin.py": {"HANDWRITTEN"},
}


def named_declarations(source: str, tables: set[str], names: set[str]):
    """The string constants in `source` that equal a name in `names`, outside
    the module-level assignments to `tables`, as (line, name)."""
    tree = ast.parse(source)
    skipped = {
        id(node)
        for statement in tree.body
        if isinstance(statement, ast.Assign)
        and any(
            isinstance(target, ast.Name) and target.id in tables
            for target in statement.targets
        )
        for node in ast.walk(statement)
    }
    return [
        (node.lineno, node.value)
        for node in ast.walk(tree)
        if isinstance(node, ast.Constant)
        and isinstance(node.value, str)
        and node.value in names
        and id(node) not in skipped
    ]


class ProtocolNameTests(unittest.TestCase):
    def test_only_protocol_and_policy_tables_name_declarations(self):
        api = real_api()
        names = {
            *(item.name for item in (*api.functions, *api.records, *api.typedefs)),
            *(enum.name for enum in api.enums),
            *(member.name for enum in api.enums for member in enum.values),
        }
        found = [
            f"{path.relative_to(GENERATOR)}:{line}: {name}"
            for path in sorted(GENERATOR.rglob("*.py"))
            if path != GENERATOR / "protocol.py"
            for line, name in named_declarations(
                path.read_text(),
                POLICY_TABLES.get(path.relative_to(GENERATOR).as_posix(), set()),
                names,
            )
        ]
        self.assertEqual(found, [], "name these declarations in protocol.py")

    def test_policy_tables_still_exist(self):
        for path, tables in POLICY_TABLES.items():
            tree = ast.parse((GENERATOR / path).read_text())
            assigned = {
                target.id
                for statement in tree.body
                if isinstance(statement, ast.Assign)
                for target in statement.targets
                if isinstance(target, ast.Name)
            }
            self.assertLessEqual(tables, assigned, path)


if __name__ == "__main__":
    unittest.main()
