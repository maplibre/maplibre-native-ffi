from __future__ import annotations

import hashlib
import io
import json
import pathlib
import re
import subprocess
import tarfile
import tempfile
import unittest
import zipfile

import go_native_modules as go
import native_package_release

from tools.ci import gaps, plan

ROOT = pathlib.Path(__file__).resolve().parents[2]
MODULE = "maplibre.org/maplibre-native-ffi/go-native/macos-arm64"
PKG_CONFIG = """prefix=${pcfiledir}/../..
libdir=${prefix}/lib
Libs: -L${libdir} -Wl,-rpath,${libdir} -lmaplibre-native-c -lz
"""
DESCRIPTOR = {
    "link": {"libraries": ["z", "c++"], "frameworks": ["Metal", "QuartzCore"]}
}


def archive(directory: pathlib.Path, headers: dict[str, bytes]) -> pathlib.Path:
    """A native archive for macos-arm64 with the given headers."""
    root = "maplibre-native-c-macos-arm64"
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
            if gaps.supported("go", plan.target_named(preset), "build")
            and not plan.target_named(preset).platform.startswith(("windows", "mingw"))
        ]

        self.assertEqual(go.presets(), expected)

    def test_packages_an_archive_as_its_presets_module(self) -> None:
        headers = go.binding_headers(ROOT)
        preset, files = go.build(
            archive(self.directory, headers), "v0.202610.2", "2026-10-01T00:00:00Z"
        )

        self.assertEqual(preset, "macos-arm64")
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

    def test_release_assets_are_reproducible_and_listed_in_the_checksums(
        self,
    ) -> None:
        archive(self.directory, go.binding_headers(ROOT))
        checksums = self.directory / "SHA256SUMS"
        checksums.write_text("0  maplibre-native-c-macos-arm64.tar.gz\n")

        def release() -> dict[str, bytes]:
            paths = go.write_release(
                self.directory,
                "v0.202612.0",
                "2026-12-01T00:00:00Z",
                ["macos-arm64"],
            )
            return {path.name: path.read_bytes() for path in paths}

        first = release()
        second = release()

        self.assertEqual(first, second)
        self.assertEqual(
            sorted(first),
            [
                f"go-native-macos-arm64-v0.202612.0.{file}"
                for file in ("info", "mod", "zip")
            ],
        )
        self.assertEqual(
            json.loads(first["go-native-macos-arm64-v0.202612.0.info"]),
            {"Version": "v0.202612.0", "Time": "2026-12-01T00:00:00Z"},
        )
        self.assertEqual(
            checksums.read_text().splitlines(),
            ["0  maplibre-native-c-macos-arm64.tar.gz"]
            + [
                f"{hashlib.sha256(content).hexdigest()}  {name}"
                for name, content in first.items()
            ],
        )

    def test_redirects_each_version_to_the_release_that_holds_its_files(
        self,
    ) -> None:
        download = "https://github.com/maplibre/maplibre-native-ffi/releases/download"
        for version, url in (
            (
                "v0.202611.0",
                f"{download}/go-native/v0.202611.0/linux-gnu-x64-v0.202611.0",
            ),
            (
                "v0.202612.3",
                f"{download}/v0.202612.3/go-native-linux-gnu-x64-v0.202612.3",
            ),
        ):
            for file in ("info", "mod", "zip"):
                with self.subTest(version=version, file=file):
                    self.assertEqual(
                        redirect(
                            go.redirects(),
                            f"/{go.MODULE_PREFIX}/linux-gnu-x64/@v/{version}.{file}",
                        ),
                        f"{url}.{file}",
                    )
        self.assertIsNone(
            redirect(go.redirects(), f"/{go.MODULE_PREFIX}/linux-gnu-x64/@v/list")
        )

    def test_the_proxy_lists_each_presets_released_versions(self) -> None:
        repository = self.directory / "repository"
        repository.mkdir()

        def git(*arguments: str) -> None:
            subprocess.run(
                [
                    "git",
                    "-c",
                    "user.name=Test",
                    "-c",
                    "user.email=test@example.com",
                    "-c",
                    "commit.gpgSign=false",
                    "-c",
                    "tag.gpgSign=false",
                    *arguments,
                ],
                cwd=repository,
                check=True,
                capture_output=True,
            )

        def release(tag: str, presets: list[str] | None) -> None:
            path = repository / go.PRESETS
            if presets is None:
                path.unlink(missing_ok=True)
            else:
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text(json.dumps(presets))
            git("add", "-A")
            git("commit", "--quiet", "--allow-empty", "-m", tag)
            git("tag", tag)

        git("init", "--quiet")
        release("v0.202610.3", None)
        release("v0.202611.0", ["linux-gnu-x64"])
        release("v0.202611.10", ["linux-gnu-x64", "macos-arm64"])
        release("v0.202611.9", ["linux-gnu-x64"])
        # Its release has not published, so its files are not out yet.
        release("v0.202611.11", ["linux-gnu-x64"])
        out = self.directory / "site"

        go.write_site(
            out,
            {"v0.202610.3", "v0.202611.0", "v0.202611.9", "v0.202611.10"},
            repository,
        )

        self.assertEqual((out / "_redirects").read_text(), go.redirects())
        self.assertEqual(
            {
                path.relative_to(out).as_posix(): path.read_text()
                for path in out.rglob("list")
            },
            {
                f"{go.MODULE_PREFIX}/linux-gnu-x64/@v/list": (
                    "v0.202611.0\nv0.202611.9\nv0.202611.10\n"
                ),
                f"{go.MODULE_PREFIX}/macos-arm64/@v/list": "v0.202611.10\n",
            },
        )


def redirect(rules: str, path: str) -> str | None:
    """Where Cloudflare Pages redirects `path`, or None. Pages applies the first
    rule that matches, and a placeholder matches anything but a slash."""
    for line in rules.splitlines():
        source, destination, status = line.split()
        assert status == "302"
        pattern = "".join(
            f"(?P<{part[1:]}>[^/]+)" if part.startswith(":") else re.escape(part)
            for part in re.split(r"(:[A-Za-z]\w*)", source)
        )
        match = re.fullmatch(pattern, path)
        if match:
            for name, value in match.groupdict().items():
                destination = destination.replace(f":{name}", value)
            return destination
    return None


if __name__ == "__main__":
    unittest.main()
