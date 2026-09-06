"""Exercise real Git changes through mise and the CI coverage policy."""

from __future__ import annotations

import json
import os
import pathlib
import shutil
import subprocess
import tempfile
import unittest
from unittest.mock import patch

from ci.affected import affected_roots, check_graph
from ci.pr_matrix import plan, select
from ci.tests.test_pr_matrix import pr
from ci.workflow import consumer_roots

ROOT = pathlib.Path(__file__).resolve().parents[2]
RUST = "bindings/rust/crates/maplibre-native-ffi"


class AffectedGraphTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        directory = tempfile.TemporaryDirectory()
        cls.addClassCleanup(directory.cleanup)
        cls.root = pathlib.Path(directory.name)
        # Copy tracked files from the working tree so graph edits are tested
        # before committing. Gitlinks stay metadata; submodules are never prepared.
        tracked = subprocess.check_output(
            ["git", "ls-files", "-z"], cwd=ROOT, text=True
        ).split("\0")
        tracked.extend(str(p.relative_to(ROOT)) for p in (ROOT / "ci").glob("*.py"))
        for name in filter(None, tracked):
            source = ROOT / name
            if source.is_file():
                target = cls.root / name
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(source, target)
        cls.trust = patch.dict(os.environ, {"MISE_TRUSTED_CONFIG_PATHS": str(cls.root)})
        cls.trust.start()
        cls.addClassCleanup(cls.trust.stop)
        cls.git("init", "--quiet")
        cls.git("add", ".")
        cls.git(
            "update-index",
            "--add",
            "--cacheinfo",
            "160000",
            "1" * 40,
            "third_party/maplibre-native",
        )
        cls.base = cls.commit()

    @classmethod
    def git(cls, *args):
        return subprocess.check_output(
            [
                "git",
                "-c",
                "core.hooksPath=/dev/null",
                "-c",
                "user.name=CI graph test",
                "-c",
                "user.email=ci-graph@example.invalid",
                *args,
            ],
            cwd=cls.root,
            text=True,
            stderr=subprocess.PIPE,
        ).strip()

    @classmethod
    def commit(cls):
        cls.git("commit", "--quiet", "--no-gpg-sign", "-m", "CI graph fixture")
        return cls.git("rev-parse", "HEAD")

    def setUp(self):
        self.git("reset", "--hard", self.base)
        self.git("clean", "-fdq")

    def changed(self, path):
        file = self.root / path
        file.parent.mkdir(parents=True, exist_ok=True)
        file.write_text((file.read_text() if file.exists() else "") + "\n")
        self.git("add", path)
        return self.commit()

    def roots(self, head, base=None):
        return affected_roots(self.root, base or self.base, head)

    def test_native_and_shared_inputs_reach_every_ci_consumer(self):
        consumers = {
            "bindings/" + name
            for name in (
                "rust",
                "kotlin",
                "swift",
                "zig",
                "dotnet",
                "python",
                "go",
                "dart",
            )
        }
        for path in (
            "include/maplibre_native_c/base.h",
            "src/platform/rust/src/lib.rs",
            "CMakePresets.json",
            "Cargo.lock",
            "mise.lock",
            "ci/workflow.py",
            "patches/new.patch",
            "unknown-root-file",
        ):
            with self.subTest(path=path):
                self.git("reset", "--hard", self.base)
                roots = self.roots(self.changed(path))
                self.assertTrue(consumers <= roots)
                self.assertIn(".", roots)
                self.assertIn("docs", roots)
        self.git("reset", "--hard", self.base)
        self.git(
            "update-index",
            "--cacheinfo",
            "160000",
            "2" * 40,
            "third_party/maplibre-native",
        )
        self.assertTrue(consumers <= self.roots(self.commit()))

    def test_binding_changes_keep_prerequisites_out_of_the_affected_set(self):
        for name, count in (
            ("python", 7),
            ("kotlin", 7),
            ("dotnet", 5),
            ("go", 5),
            ("swift", 3),
            ("zig", 7),
            ("dart", 7),
        ):
            with self.subTest(binding=name):
                self.git("reset", "--hard", self.base)
                roots = self.roots(self.changed(f"bindings/{name}/mise.toml"))
                self.assertIn(f"bindings/{name}", roots)
                self.assertIn("docs", roots)
                self.assertNotIn(".", roots)
                self.assertNotIn("bindings/rust", roots)
                result = plan("pull_request", pr(), roots)["expected"]
                self.assertEqual(
                    sum(
                        k.startswith("target-") and v == "success"
                        for k, v in result.items()
                    ),
                    count,
                )

    def test_cargo_inference_and_shared_rust_task_root(self):
        for crate in ("-core", "-sys", ""):
            with self.subTest(crate=crate):
                self.git("reset", "--hard", self.base)
                roots = self.roots(self.changed(f"{RUST}{crate}/src/lib.rs"))
                self.assertIn("bindings/rust", roots)
                self.assertIn("examples/rust-map", roots)
                self.assertEqual("bindings/python" in roots, bool(crate))
                self.assertNotIn(".", roots)
        self.git("reset", "--hard", self.base)
        manifest = self.root / "bindings/python/Cargo.toml"
        manifest.write_text(
            manifest.read_text().replace(
                "[dependencies]",
                '[dependencies]\nmaplibre-native-ffi = { path = "../rust/crates/maplibre-native-ffi" }',
                1,
            )
        )
        self.git("add", "bindings/python/Cargo.toml")
        updated_dependencies = self.commit()
        head = self.changed(f"{RUST}/src/lib.rs")
        self.assertIn("bindings/python", self.roots(head, updated_dependencies))

    def test_docs_examples_and_gradle_follow_their_owners(self):
        for path, expected in (
            ("docs/src/content/docs/concepts.md", {"docs"}),
            ("docs/snippets/c/new.c", {"docs/snippets", "docs"}),
            ("examples/android-map/mise.toml", {"examples/android-map"}),
            ("examples/rust-map/src/main.rs", {"examples/rust-map"}),
        ):
            with self.subTest(path=path):
                self.git("reset", "--hard", self.base)
                self.assertEqual(self.roots(self.changed(path)), expected)
        self.git("reset", "--hard", self.base)
        roots = self.roots(self.changed("gradle/libs.versions.toml"))
        self.assertTrue(
            {
                "bindings/kotlin",
                "examples/android-map",
                "examples/compose-map",
                "examples/lwjgl-map",
                "docs",
            }
            <= roots
        )
        self.assertNotIn("bindings/python", roots)

    def test_empty_diff_deletion_rename_and_removed_project(self):
        self.assertEqual(self.roots(self.base), set())
        self.git("rm", "bindings/go/mise.toml")
        self.assertIn("bindings/go", self.roots(self.commit()))
        self.git("reset", "--hard", self.base)
        self.git("mv", "bindings/go/mise.toml", "bindings/swift/from-go.toml")
        self.assertTrue({"bindings/go", "bindings/swift"} <= self.roots(self.commit()))
        self.git("reset", "--hard", self.base)
        self.git("rm", "-r", "examples/go-map")
        config = self.root / "mise.toml"
        config.write_text(
            config.read_text().replace(
                '[monorepo.projects."example:go-map"]\nroot = "examples/go-map"\ndepends = ["ffi:go"]\n',
                "",
            )
        )
        self.git("add", "mise.toml")
        head = self.commit()
        with self.assertRaisesRegex(ValueError, "examples/go-map"):
            self.roots(head)
        workflow = self.root / "ci/workflow.toml"
        workflow.write_text(
            "\n".join(
                line
                for line in workflow.read_text().splitlines()
                if "//examples/go-map:" not in line
            )
            + "\n"
        )
        self.git("add", "ci/workflow.toml")
        self.assertIn(".", self.roots(self.commit()))

    def test_explicit_merge_revision_excludes_already_merged_base_changes(self):
        self.git("checkout", "-B", "pr", self.base)
        head = self.changed("bindings/python/mise.toml")
        self.git("checkout", "-B", "main", self.base)
        base = self.changed("src/new.cpp")
        self.git("merge", "--no-edit", "--no-gpg-sign", head)
        merged = self.git("rev-parse", "HEAD")
        self.assertEqual(self.roots(merged, base), {"bindings/python", "docs"})

    def test_missing_history_and_unmapped_consumers_fail_selection(self):
        with self.assertRaises(subprocess.CalledProcessError):
            self.roots("f" * 40)
        with (
            patch("ci.affected.consumer_roots", return_value={"bindings/newlang"}),
            self.assertRaisesRegex(ValueError, "missing from mise graph"),
        ):
            check_graph(self.root)

    def test_plan_entrypoint_emits_affected_jobs_and_cache_writers(self):
        head = self.changed("bindings/python/mise.toml")
        event = pr()
        event["pull_request"]["base"] = {"sha": self.base}
        with tempfile.TemporaryDirectory() as directory:
            output = pathlib.Path(directory)
            (output / "event.json").write_text(json.dumps(event))
            subprocess.run(
                ["bash", str(self.root / ".mise/tasks/ci/plan")],
                cwd=output,
                check=True,
                env={
                    "PATH": os.environ["PATH"],
                    "MISE_TRUSTED_CONFIG_PATHS": str(self.root),
                    "GITHUB_EVENT_NAME": "pull_request",
                    "GITHUB_EVENT_PATH": str(output / "event.json"),
                    "GITHUB_SHA": head,
                    "GITHUB_OUTPUT": str(output / "output"),
                    "GITHUB_STEP_SUMMARY": str(output / "summary"),
                },
            )
            values = dict(
                line.split("=", 1)
                for line in (output / "output").read_text().splitlines()
            )
            expected = plan("pull_request", event, {"bindings/python", "docs"})
            self.assertEqual(json.loads(values["expected"]), expected["expected"])
            self.assertEqual(
                json.loads(values["toolchain_writers"]), expected["toolchain_writers"]
            )
            self.assertIn("Affected project roots:", (output / "summary").read_text())


class SelectionValidationTest(unittest.TestCase):
    def test_malformed_or_partial_graph_cannot_become_an_empty_selection(self):
        for graph in (
            {},
            {"projects": None},
            {"projects": []},
            {"projects": [{"id": "native", "root": "../outside"}]},
            {"projects": [{"id": "native", "root": ".", "dependencies": ["missing"]}]},
        ):
            with (
                self.subTest(graph=graph),
                patch("ci.affected.mise_json", return_value=graph),
                self.assertRaises((TypeError, ValueError)),
            ):
                check_graph(ROOT)
        for selection in (
            {},
            {
                "base": "a" * 40,
                "head": "b" * 40,
                "projects": [{"id": "unknown", "root": "bindings/python"}],
            },
        ):
            with (
                patch("ci.affected.check_graph", return_value={"native": "."}),
                patch("ci.affected.mise_json", return_value=selection),
                self.assertRaises((TypeError, ValueError)),
            ):
                affected_roots(ROOT, "a" * 40, "b" * 40)

    def test_selection_errors_retain_the_tier_and_requested_platforms(self):
        event = pr(True, ("ci:android",))
        event["pull_request"]["base"] = {"sha": "a" * 40}
        for error in (
            ValueError("invalid graph"),
            TypeError("invalid graph schema"),
            FileNotFoundError("mise"),
            subprocess.CalledProcessError(1, ["mise"]),
            subprocess.TimeoutExpired(["mise"], 60),
        ):
            with (
                self.subTest(error=error),
                patch("ci.pr_matrix.affected_roots", side_effect=error),
            ):
                selection, explanation = select("pull_request", event, "b" * 40)
                self.assertEqual(selection, plan("pull_request", event))
                self.assertIn("retaining the complete tier", explanation)
        with patch(
            "ci.pr_matrix.affected_roots",
            side_effect=AssertionError("unexpected query"),
        ):
            selection, _ = select("pull_request", pr(True, ("ci:full",)), "")
            self.assertEqual(selection["tier"], "full")

    def test_consumer_mapping_includes_nested_tasks_and_rejects_unknown_syntax(self):
        with patch(
            "ci.workflow.consumer_commands",
            return_value=["mise run //bindings/dart:build:mobile android-x64-egl"],
        ):
            self.assertEqual(consumer_roots({}, "unused"), {"bindings/dart"})
        for command in (
            "echo skipped",
            "mise run unscoped",
            "mise run //bindings/newlang:",
        ):
            with (
                patch("ci.workflow.consumer_commands", return_value=[command]),
                self.assertRaises(ValueError),
            ):
                consumer_roots({}, "unused")
