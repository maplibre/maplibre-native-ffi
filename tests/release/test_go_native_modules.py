from __future__ import annotations

import io
import json
import pathlib
import tarfile
import tempfile
import unittest
import zipfile

import go_native_modules as go
import native_package_release

from tools.ci import gaps

ROOT = pathlib.Path(__file__).resolve().parents[2]
MODULE = "maplibre.org/maplibre-native-ffi/go-native/macos-arm64-metal"
PKG_CONFIG = """prefix=${pcfiledir}/../..
libdir=${prefix}/lib
Libs: -L${libdir} -Wl,-rpath,${libdir} -lmaplibre-native-c -lz
"""
DESCRIPTOR = {
    "link": {"libraries": ["z", "c++"], "frameworks": ["Metal", "QuartzCore"]}
}


def archive(directory: pathlib.Path, headers: dict[str, bytes]) -> pathlib.Path:
    """A native archive for macos-arm64-metal with the given headers."""
    root = "maplibre-native-c-macos-arm64-metal"
    files = {
        "share/pkgconfig/maplibre-native-c.pc": PKG_CONFIG.encode(),
        "share/maplibre-native-c/artifact.json": json.dumps(DESCRIPTOR).encode(),
        "share/maplibre-native-c/LICENSE": b"license",
        "share/maplibre-native-c/licenses/zlib.txt": b"zlib",
        "lib/libmaplibre-native-c.a": b"!<arch>\n",
        "lib/libmaplibre-native-c.dylib": b"shared",
    }
    for path, content in headers.items():
        files[f"include/{path}"] = content
    path = directory / f"{root}.tar.gz"
    with tarfile.open(path, "w:gz") as output:
        for name, content in files.items():
            info = tarfile.TarInfo(f"{root}/{name}")
            info.size = len(content)
            output.addfile(info, io.BytesIO(content))
    return path


class GoNativeModulesTest(unittest.TestCase):
    def setUp(self) -> None:
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.directory = pathlib.Path(temporary.name)

    def test_publishes_every_released_preset_that_go_can_link(self) -> None:
        # Windows links through pkg-config until CI links a MinGW preset module,
        # whose archives need LLD.
        expected = [
            preset
            for preset in native_package_release.package_presets(ROOT)
            if gaps.gap("go", gaps.target_from_name(preset), "build") is None
            and not gaps.target_from_name(preset).platform.startswith("windows")
        ]

        self.assertEqual(go.presets(), expected)

    def test_packages_an_archive_as_its_presets_module(self) -> None:
        headers = go.binding_headers(ROOT)
        preset, files = go.build(
            archive(self.directory, headers), "v0.202610.2", "2026-10-01T00:00:00Z"
        )

        self.assertEqual(preset, "macos-arm64-metal")
        self.assertEqual(
            files["v0.202610.2.mod"], f"module {MODULE}\n\ngo 1.26\n".encode()
        )
        with zipfile.ZipFile(io.BytesIO(files["v0.202610.2.zip"])) as module:
            names = module.namelist()
            native = module.read(f"{MODULE}@v0.202610.2/native.go").decode()
        self.assertEqual(
            sorted(names),
            [
                f"{MODULE}@v0.202610.2/{name}"
                for name in (
                    "LICENSE",
                    "doc.go",
                    "go.mod",
                    "libmaplibre-native-c.a",
                    "licenses/zlib.txt",
                    "native.go",
                )
            ],
        )
        self.assertIn("//go:build darwin && !ios && arm64 && !mln_pkgconfig", native)
        self.assertIn(
            "#cgo LDFLAGS: ${SRCDIR}/libmaplibre-native-c.a -lz -lc++ "
            "-framework Metal -framework QuartzCore\n",
            native,
        )
        self.assertIn('const char mln_go_preset_version[] = "v0.202610.2";', native)

    def test_rejects_an_archive_with_other_headers(self) -> None:
        headers = go.binding_headers(ROOT)
        headers["maplibre_native_c/base.h"] = b"// another release\n"

        with self.assertRaises(SystemExit):
            go.build(archive(self.directory, headers), "v0.202610.2", "")

    def test_lists_versions_in_release_order(self) -> None:
        self.assertEqual(
            go.versions_list("v0.202610.9\nv0.202609.0\n", "v0.202610.10"),
            "v0.202609.0\nv0.202610.9\nv0.202610.10\n",
        )


if __name__ == "__main__":
    unittest.main()
