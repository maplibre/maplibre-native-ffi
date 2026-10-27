from __future__ import annotations

import hashlib
import os
import pathlib
import subprocess
import tempfile
import unittest

from native_package_release import XCFRAMEWORK_NAME

REPOSITORY = pathlib.Path(__file__).resolve().parents[2]
SCRIPT = REPOSITORY / "tools/release/publish-distribution.sh"
DIGEST = "539f11cb25fe35a1e8b8e8b6f1c095cde9f93f0f8efddb4115469335d6d37b8a"
# A release runner has no Git configuration of its own, so neither do the runs
# here.
ENVIRONMENT = os.environ | {
    "GIT_CONFIG_GLOBAL": os.devnull,
    "GIT_CONFIG_NOSYSTEM": "1",
}


def git(repository: pathlib.Path, *arguments: str) -> str:
    return subprocess.run(
        ["git", "-C", str(repository), *arguments],
        check=True,
        capture_output=True,
        text=True,
        env=ENVIRONMENT,
    ).stdout.strip()


class PublishDistributionTest(unittest.TestCase):
    def setUp(self) -> None:
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        root = pathlib.Path(temporary.name)
        self.assets = root / "assets"
        self.assets.mkdir()
        archive = self.assets / "maplibre-native-c-macos-arm64-metal.tar.gz"
        archive.write_bytes(b"native archive")
        archive_digest = hashlib.sha256(archive.read_bytes()).hexdigest()
        (self.assets / "SHA256SUMS").write_text(
            f"{archive_digest}  {archive.name}\n{DIGEST}  {XCFRAMEWORK_NAME}\n"
        )
        self.remote = root / "distribution.git"
        subprocess.run(
            ["git", "init", "--quiet", "--bare", "--initial-branch=main", self.remote],
            check=True,
            env=ENVIRONMENT,
        )

    def publish(self, tag: str) -> subprocess.CompletedProcess[str]:
        return subprocess.run(
            ["bash", SCRIPT, str(self.remote), tag, str(self.assets)],
            check=False,
            capture_output=True,
            text=True,
            env=ENVIRONMENT,
        )

    def test_each_release_commits_on_main_and_tags_its_commit(self) -> None:
        self.assertEqual(self.publish("v0.202611.0").returncode, 0)
        self.assertEqual(self.publish("v0.202611.1").returncode, 0)

        self.assertEqual(
            git(self.remote, "rev-parse", "v0.202611.1^{commit}"),
            git(self.remote, "rev-parse", "main"),
        )
        self.assertEqual(
            git(self.remote, "rev-parse", "main^"),
            git(self.remote, "rev-parse", "v0.202611.0^{commit}"),
        )
        self.assertIn(
            "/releases/download/v0.202611.1/",
            git(self.remote, "show", "v0.202611.1:Package.swift"),
        )

    def test_a_rerun_after_a_publish_changes_nothing(self) -> None:
        self.publish("v0.202611.0")
        main = git(self.remote, "rev-parse", "main")

        rerun = self.publish("v0.202611.0")

        self.assertEqual(rerun.returncode, 0, rerun.stderr)
        self.assertIn("already published", rerun.stdout)
        self.assertEqual(git(self.remote, "rev-parse", "main"), main)

    def test_a_rerun_tags_the_commit_that_an_earlier_attempt_pushed(self) -> None:
        self.publish("v0.202611.0")
        main = git(self.remote, "rev-parse", "main")
        git(self.remote, "tag", "--delete", "v0.202611.0")

        rerun = self.publish("v0.202611.0")

        self.assertEqual(rerun.returncode, 0, rerun.stderr)
        self.assertEqual(git(self.remote, "rev-parse", "main"), main)
        self.assertEqual(git(self.remote, "rev-parse", "v0.202611.0^{commit}"), main)

    def test_a_tag_that_names_another_tree_fails(self) -> None:
        self.publish("v0.202611.0")
        (self.assets / "SHA256SUMS").write_text(
            (self.assets / "SHA256SUMS").read_text().replace(DIGEST, "0" * 64)
        )

        rerun = self.publish("v0.202611.0")

        self.assertNotEqual(rerun.returncode, 0)
        self.assertIn("already names another tree", rerun.stderr)


if __name__ == "__main__":
    unittest.main()
