from __future__ import annotations

import json
import pathlib
import tempfile
import unittest

from python_release import BACKENDS, license_runtime, pack, prepare
from wheel.wheelfile import WheelFile

API = """\
[project]
name = "maplibre-native-ffi"
version = "0.0.0"
license = "BSD-2-Clause"
license-files = ["LICENSE"]
"""

RUNTIME = """\
[project]
name = "maplibre-native-ffi-runtime-{backend}"
version = "0.0.0"
license = "BSD-2-Clause"
license-files = ["LICENSE", "licenses/*"]
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

    def test_the_api_distribution_takes_the_version(self) -> None:
        prepare(self.package, "0.202610.2.dev202610221817")

        self.assertEqual(
            (self.package / "pyproject.toml").read_text(),
            '[project]\nname = "maplibre-native-ffi"\n'
            'version = "0.202610.2.dev202610221817"\n'
            'license = "BSD-2-Clause"\nlicense-files = ["LICENSE"]\n',
        )
        self.assertIn("BSD", (self.package / "LICENSE").read_text())

    def test_the_repository_api_matches_what_it_stamps(self) -> None:
        source = pathlib.Path(__file__).resolve().parents[2] / "bindings/python"
        (self.package / "pyproject.toml").write_text(
            (source / "pyproject.toml").read_text()
        )

        prepare(self.package, "0.202610.2")

    def test_the_prefix_backend_runtime_takes_the_native_notices(self) -> None:
        self.describe("metal")

        license_runtime(self.package, self.prefix)

        runtimes = self.package / "runtimes"
        self.assertEqual((runtimes / "metal/licenses/icu.txt").read_text(), "ICU")
        self.assertFalse((runtimes / "opengl/licenses").exists())
        self.assertFalse((runtimes / "vulkan/licenses").exists())
        self.assertIn("BSD", (runtimes / "metal/LICENSE").read_text())
        self.assertEqual(
            (runtimes / "metal/pyproject.toml").read_text(),
            RUNTIME.format(backend="metal"),
        )

    def test_rejects_a_backend_without_a_runtime(self) -> None:
        self.describe("webgpu")

        with self.assertRaisesRegex(SystemExit, "no Python runtime serves"):
            license_runtime(self.package, self.prefix)

    def test_rejects_a_version_outside_the_release_scheme(self) -> None:
        with self.assertRaisesRegex(SystemExit, "invalid release version"):
            prepare(self.package, "0.1.0.dev202610221817")


class RuntimeWheelTest(unittest.TestCase):
    def test_versions_metadata_preserving_the_extension_tags_and_licenses(self):
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            inputs, output = root / "inputs", root / "output"
            inputs.mkdir()
            name = "maplibre_native_ffi_runtime_opengl"
            tags = "cp314-cp314-android_27_arm64_v8a"
            stem = f"{name}-0.0.0"
            wheel = inputs / f"{stem}-{tags}.whl"
            files = {
                f"{name}/_native.so": b"tested extension",
                f"{stem}.dist-info/METADATA": (
                    b"Metadata-Version: 2.4\n"
                    b"Name: maplibre-native-ffi-runtime-opengl\n"
                    b"Version: 0.0.0\nRequires-Dist: maplibre-native-ffi==0.0.0\n"
                ),
                f"{stem}.dist-info/WHEEL": f"Wheel-Version: 1.0\nTag: {tags}\n".encode(),
                f"{stem}.dist-info/licenses/icu.txt": b"ICU notices",
            }
            with WheelFile(wheel, "w") as source:
                for path, contents in files.items():
                    source.writestr(path, contents)
            for version in ("0.202610.3", "0.202611.0.dev202610291234"):
                pack(inputs, output, version)
                with WheelFile(output / f"{name}-{version}-{tags}.whl") as stamped:
                    # WheelFile checks every RECORD hash as it reads the contents.
                    for path, contents in files.items():
                        if path.endswith("/METADATA"):
                            contents = contents.replace(b"0.0.0", version.encode())
                        self.assertEqual(
                            stamped.read(
                                path.replace(
                                    "-0.0.0.dist-info/", f"-{version}.dist-info/"
                                )
                            ),
                            contents,
                        )

    def test_missing_runtime_artifacts_fail_packing(self):
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            with self.assertRaisesRegex(SystemExit, "no runtime wheels"):
                pack(root, root / "output", "0.202610.3")


if __name__ == "__main__":
    unittest.main()
