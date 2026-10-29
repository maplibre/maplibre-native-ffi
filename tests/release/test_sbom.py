from __future__ import annotations

import json
import pathlib
import subprocess
import tempfile
import unittest

from sbom import SBOM_NAME, native


class NativeSbomTests(unittest.TestCase):
    def test_inventory_and_notices_describe_the_same_selected_components(self):
        with tempfile.TemporaryDirectory() as temporary:
            directory = pathlib.Path(temporary)
            source = directory / "source"
            source.mkdir()
            for arguments in (
                ["init", "--quiet"],
                [
                    "-c",
                    "user.name=SBOM Test",
                    "-c",
                    "user.email=sbom@example.com",
                    "-c",
                    "commit.gpgsign=false",
                    "commit",
                    "--quiet",
                    "--allow-empty",
                    "-m",
                    "source",
                ],
            ):
                subprocess.run(
                    ["git", "-C", str(source), *arguments],
                    check=True,
                    capture_output=True,
                )
            notice = source / "LICENSE"
            notice.write_text("Dependency copyright and terms", encoding="utf-8")
            records = [
                {
                    "name": "dependency",
                    "version": "",
                    "source": str(source),
                    "url": "http://127.0.0.1/dependency",
                    "license": "MIT",
                    "parent": "",
                    "notices": {"dependency.txt": str(notice)},
                }
            ]
            artifact = {
                "version": "0.202610.0",
                "commit": "abcdef",
                "platform": {"id": "linux-gnu-x64"},
                "backend": "vulkan",
                "link": {"libraries": ["m"], "frameworks": []},
            }
            output = directory / "output"
            result = native(records, artifact, output)
            self.assertEqual(json.loads((output / SBOM_NAME).read_text()), result)
            self.assertEqual(
                (output / "licenses/dependency.txt").read_text(), notice.read_text()
            )
            dependency, system = result["components"]
            self.assertEqual(dependency["name"], "dependency")
            self.assertEqual(len(dependency["version"]), 40)
            self.assertEqual(dependency["licenses"], [{"expression": "MIT"}])
            self.assertEqual(system["scope"], "excluded")
            root = result["metadata"]["component"]
            graph = {
                entry["ref"]: entry["dependsOn"] for entry in result["dependencies"]
            }
            self.assertEqual(
                set(graph[root["bom-ref"]]), {dependency["bom-ref"], system["bom-ref"]}
            )
            self.assertNotIn(temporary, json.dumps(result))

            native([], artifact, output)
            self.assertFalse((output / "licenses/dependency.txt").exists())
            self.assertNotIn(
                "dependency",
                {
                    item["name"]
                    for item in json.loads((output / SBOM_NAME).read_text())[
                        "components"
                    ]
                },
            )
