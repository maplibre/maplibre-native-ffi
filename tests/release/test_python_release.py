from __future__ import annotations

import json
import pathlib
import tempfile
import unittest

from python_release import BACKENDS, prepare

API = """\
[project]
name = "maplibre-native-ffi"
version = "0.0.0"
license = "BSD-2-Clause"
"""

RUNTIME = """\
[project]
name = "maplibre-native-ffi-runtime-{backend}"
version = "0.0.0"
license = "BSD-2-Clause"
dependencies = ["maplibre-native-ffi==0.0.0"]
"""


class PythonReleaseTest(unittest.TestCase):
    def setUp(self) -> None:
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        root = pathlib.Path(temporary.name)
        self.package = root / "package"
        self.package.mkdir()
        (self.package / "pyproject.toml").write_text(API)
        for backend in BACKENDS:
            runtime = self.package / "runtimes" / backend
            runtime.mkdir(parents=True)
            (runtime / "pyproject.toml").write_text(RUNTIME.format(backend=backend))
        self.prefix = root / "prefix"
        (self.prefix / "share/maplibre-native-c/licenses").mkdir(parents=True)
        (self.prefix / "share/maplibre-native-c/licenses/icu.txt").write_text("ICU")

    def describe(self, backend: str) -> None:
        (self.prefix / "share/maplibre-native-c/artifact.json").write_text(
            json.dumps({"platform": {"id": "macos-arm64"}, "backend": backend})
        )

    def test_every_distribution_takes_the_version_and_every_runtime_pins_the_api(
        self,
    ) -> None:
        prepare(self.package, "0.202610.2.dev202610221817")

        self.assertEqual(
            (self.package / "pyproject.toml").read_text(),
            '[project]\nname = "maplibre-native-ffi"\n'
            'version = "0.202610.2.dev202610221817"\n'
            'license = "BSD-2-Clause"\nlicense-files = ["LICENSE"]\n',
        )
        self.assertIn("BSD", (self.package / "LICENSE").read_text())
        for backend in BACKENDS:
            runtime = self.package / "runtimes" / backend
            self.assertEqual(
                (runtime / "pyproject.toml").read_text(),
                f'[project]\nname = "maplibre-native-ffi-runtime-{backend}"\n'
                'version = "0.202610.2.dev202610221817"\n'
                'license = "BSD-2-Clause"\n'
                'license-files = ["LICENSE", "licenses/*"]\n'
                'dependencies = ["maplibre-native-ffi==0.202610.2.dev202610221817"]\n',
            )
            self.assertIn("BSD", (runtime / "LICENSE").read_text())
            self.assertFalse((runtime / "licenses").exists())

    def test_the_repository_projects_match_what_it_stamps(self) -> None:
        source = pathlib.Path(__file__).resolve().parents[2] / "bindings/python"
        for project in ("", *(f"runtimes/{backend}" for backend in BACKENDS)):
            (self.package / project / "pyproject.toml").write_text(
                (source / project / "pyproject.toml").read_text()
            )

        prepare(self.package, "0.202610.2")

    def test_the_prefix_backend_runtime_takes_the_native_notices(self) -> None:
        self.describe("metal")

        prepare(self.package, "0.202610.2", self.prefix)

        runtimes = self.package / "runtimes"
        self.assertEqual((runtimes / "metal/licenses/icu.txt").read_text(), "ICU")
        self.assertFalse((runtimes / "opengl/licenses").exists())
        self.assertFalse((runtimes / "vulkan/licenses").exists())

    def test_rejects_a_backend_without_a_runtime(self) -> None:
        self.describe("webgpu")

        with self.assertRaisesRegex(SystemExit, "no Python runtime serves"):
            prepare(self.package, "0.202610.2", self.prefix)

    def test_rejects_a_version_outside_the_release_scheme(self) -> None:
        with self.assertRaisesRegex(SystemExit, "invalid release version"):
            prepare(self.package, "0.1.0.dev202610221817")


if __name__ == "__main__":
    unittest.main()
