from __future__ import annotations

import hashlib
import json
import pathlib
import subprocess
import tempfile
import unittest

from distribution import ROOT, Release, write
from vcpkg_registry import PORT


class VcpkgRegistryTest(unittest.TestCase):
    def setUp(self) -> None:
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        root = pathlib.Path(temporary.name)
        self.assets = root / "assets"
        self.assets.mkdir()
        self.archive = self.assets / f"{PORT}-linux-gnu-x64-egl.tar.gz"
        self.archive.write_bytes(b"native archive")
        digest = hashlib.sha256(self.archive.read_bytes()).hexdigest()
        (self.assets / "SHA256SUMS").write_text(f"{digest}  {self.archive.name}\n")
        self.tree = root / "registry"
        self.tree.mkdir()
        self.git("init", "--quiet")
        self.git("config", "user.name", "Registry test")
        self.git("config", "user.email", "registry@example.invalid")
        self.git("config", "commit.gpgSign", "false")

    def git(self, *arguments: str) -> str:
        return subprocess.check_output(
            ["git", "-C", str(self.tree), *arguments], text=True
        ).strip()

    def publish(self, version: str) -> None:
        # No XCFramework is needed to exercise the registry writer, while the
        # distribution tree's cleanup must preserve its version history.
        from unittest.mock import patch

        import distribution
        import vcpkg_registry

        with patch.object(distribution, "WRITERS", (vcpkg_registry.write,)):
            write(self.tree, Release(self.assets, f"v{version}"), ROOT)
        self.git("add", "--all")
        self.git("commit", "--quiet", "-m", version)

    def versions(self) -> list[dict]:
        return json.loads((self.tree / f"versions/m-/{PORT}.json").read_text())[
            "versions"
        ]

    def test_each_version_names_the_committed_port_tree_and_remains_available(
        self,
    ) -> None:
        self.publish("0.202610.0")
        first = self.git("rev-parse", f"HEAD:ports/{PORT}")
        self.assertEqual(self.versions()[0]["git-tree"], first)

        self.publish("0.202610.1")

        self.assertEqual(
            [entry["version"] for entry in self.versions()],
            ["0.202610.1", "0.202610.0"],
        )
        self.assertEqual(
            self.versions()[0]["git-tree"], self.git("rev-parse", f"HEAD:ports/{PORT}")
        )
        self.assertEqual(self.versions()[1]["git-tree"], first)
        self.assertEqual(
            json.loads(self.git("show", f"{first}:vcpkg.json"))["version"], "0.202610.0"
        )
        self.assertEqual(
            json.loads((self.tree / "versions/baseline.json").read_text()),
            {
                "default": {PORT: {"baseline": "0.202610.1", "port-version": 0}},
            },
        )

    def test_release_urls_pin_the_sha512_of_the_native_archive(self) -> None:
        self.publish("0.202610.0")

        selections = (self.tree / f"ports/{PORT}/archives.cmake").read_text()
        self.assertIn(f"/releases/download/v0.202610.0/{self.archive.name}", selections)
        self.assertIn(hashlib.sha512(self.archive.read_bytes()).hexdigest(), selections)
        self.assertIn('"${VCPKG_CMAKE_SYSTEM_NAME}" STREQUAL "Linux"', selections)
        self.assertIn('VCPKG_TARGET_ARCHITECTURE STREQUAL "x64"', selections)
        self.assertIn('backend STREQUAL "opengl"', selections)

    def test_an_archive_that_does_not_match_the_release_checksum_fails(self) -> None:
        self.archive.write_bytes(b"corrupted")

        with self.assertRaisesRegex(SystemExit, "does not match SHA256SUMS"):
            self.publish("0.202610.0")

    def test_an_existing_version_cannot_name_a_different_port(self) -> None:
        self.publish("0.202610.0")
        self.archive.write_bytes(b"different archive")
        digest = hashlib.sha256(self.archive.read_bytes()).hexdigest()
        (self.assets / "SHA256SUMS").write_text(f"{digest}  {self.archive.name}\n")

        with self.assertRaisesRegex(SystemExit, "already names another port tree"):
            self.publish("0.202610.0")

    def test_no_renderer_or_multiple_renderers_fail_before_downloading(self) -> None:
        self.publish("0.202610.0")
        for features in ("", "opengl;vulkan"):
            with self.subTest(features=features):
                result = subprocess.run(
                    [
                        "cmake",
                        f"-DFEATURES={features}",
                        "-P",
                        str(self.tree / f"ports/{PORT}/portfile.cmake"),
                    ],
                    capture_output=True,
                    text=True,
                    check=False,
                )
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("renderer feature", result.stderr)


if __name__ == "__main__":
    unittest.main()
