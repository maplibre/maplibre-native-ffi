"""Exercise coverage transitions, workflow dependencies, and the merge gate."""

from __future__ import annotations

import itertools
import json
import os
import pathlib
import re
import subprocess
import tempfile
import textwrap
import unittest

from ci.pr_matrix import PLATFORM_GROUPS, plan

ROOT = pathlib.Path(__file__).resolve().parents[2]
WORKFLOW = ROOT / ".github/workflows/ci.yml"


def pr(draft=False, labels=(), author="contributor", action="synchronize"):
    return {
        "action": action,
        "sender": {"login": "maintainer"},
        "pull_request": {
            "draft": draft,
            "labels": [{"name": label} for label in labels],
            "user": {"login": author},
        },
    }


class CoverageTest(unittest.TestCase):
    def test_readiness_and_labels_expand_and_restore_coverage(self):
        draft = plan("pull_request", pr(True))["expected"]
        ready = plan("pull_request", pr(action="ready_for_review"))["expected"]
        draft_targets = {
            k for k, v in draft.items() if k.startswith("target-") and v == "success"
        }
        ready_targets = {
            k for k, v in ready.items() if k.startswith("target-") and v == "success"
        }
        self.assertEqual(
            draft_targets, {"target-linux-gnu-x64-egl", "target-linux-gnu-x64-vulkan"}
        )
        self.assertEqual(
            ready_targets - draft_targets,
            {
                "target-macos-arm64-metal",
                "target-windows-x64-wgl",
                "target-windows-x64-vulkan",
                "target-android-x64-egl",
                "target-android-x64-vulkan",
                "target-emscripten-wasm32-webgl",
                "target-emscripten-wasm32-webgpu",
            },
        )
        for draft_state in (True, False):
            for action in (
                "labeled",
                "synchronize",
                "converted_to_draft",
                "ready_for_review",
            ):
                full = plan(
                    "pull_request", pr(draft_state, ("ci:full",), action=action)
                )
                self.assertEqual(full["tier"], "full")
                self.assertEqual(set(full["expected"].values()), {"success"})
            removed = plan(
                "pull_request", pr(draft_state, ("unrelated",), action="unlabeled")
            )
            self.assertEqual(removed["expected"], draft if draft_state else ready)

    def test_dependabot_authorship_main_and_manual_always_select_full(self):
        for name, event in [
            ("push", {}),
            ("workflow_dispatch", {}),
            ("pull_request", pr(True, author="dependabot[bot]", action="unlabeled")),
            ("pull_request", pr(False, author="dependabot[bot]")),
        ]:
            selection = plan(name, event)
            self.assertEqual(selection["tier"], "full")
            self.assertEqual(set(selection["expected"].values()), {"success"})
        event = pr(True)
        event["sender"]["login"] = "dependabot[bot]"
        self.assertEqual(plan("pull_request", event)["tier"], "draft")
        for invalid in ({}, {"pull_request": {}}, {"pull_request": {"labels": []}}):
            with self.assertRaises(KeyError):
                plan("pull_request", invalid)

    def test_platform_groups_combine_and_satisfy_packaging_dependencies(self):
        workflow = WORKFLOW.read_text()
        blocks = dict(
            re.findall(
                r"^  ([\w-]+):\n(.*?)(?=^  [\w-]+:|\Z)",
                workflow.split("jobs:\n", 1)[1],
                re.MULTILINE | re.DOTALL,
            )
        )
        full = plan("push", {})["expected"]
        self.assertEqual(set(full), set(blocks) - {"required"})
        required_needs = set(
            re.findall(r"^      - ([\w-]+)$", blocks["required"], re.MULTILINE)
        )
        self.assertEqual(required_needs, set(full))
        for job in ("hygiene", "docs"):
            self.assertNotIn("needs:", blocks[job])
        for count in range(len(PLATFORM_GROUPS) + 1):
            for labels in itertools.combinations(PLATFORM_GROUPS, count):
                expected = plan("pull_request", pr(True, labels))["expected"]
                for job, wanted in expected.items():
                    block = blocks[job]
                    if job.startswith("target-") or job in {
                        "android-multi",
                        "kotlin-maven",
                    }:
                        self.assertIn(
                            f"if: fromJSON(needs.plan.outputs.expected)['{job}'] == 'success'",
                            block,
                        )
                    if wanted == "success":
                        for dependency in re.findall(
                            r"^      - ([\w-]+)$", block, re.MULTILINE
                        ):
                            self.assertEqual(
                                expected[dependency],
                                "success",
                                (labels, job, dependency),
                            )
                self.assertEqual(expected["kotlin-maven"], "skipped")
                self.assertEqual(
                    expected["android-multi"],
                    "success" if "ci:android" in labels else "skipped",
                )
        for label, representative in {
            "ci:apple": "tvos-simulator-arm64-metal",
            "ci:android": "android-arm-egl",
            "ci:linux": "linux-musl-arm64-vulkan",
            "ci:windows": "windows-arm64-wgl",
            "ci:ohos": "ohos-x64-egl",
        }.items():
            expected = plan("pull_request", pr(True, (label,)))["expected"]
            self.assertEqual(expected[f"target-{representative}"], "success")

    def test_entrypoint_runs_outside_checkout_and_emits_the_plan(self):
        event = pr(True, ("ci:apple", "ci:android"))
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            (root / "event").write_text(json.dumps(event))
            subprocess.run(
                ["bash", str(ROOT / ".mise/tasks/ci/plan")],
                cwd=root,
                check=True,
                env={
                    "PATH": os.environ["PATH"],
                    "GITHUB_EVENT_NAME": "pull_request",
                    "GITHUB_EVENT_PATH": str(root / "event"),
                    "GITHUB_OUTPUT": str(root / "output"),
                    "GITHUB_STEP_SUMMARY": str(root / "summary"),
                },
            )
            outputs = dict(
                line.split("=", 1)
                for line in (root / "output").read_text().splitlines()
            )
            self.assertEqual(
                json.loads(outputs["expected"]), plan("pull_request", event)["expected"]
            )
            self.assertIn("target-ios-arm64-metal", (root / "summary").read_text())
        events = (
            WORKFLOW.read_text()
            .split("  pull_request:\n", 1)[1]
            .split("  workflow_dispatch:", 1)[0]
        )
        for action in (
            "opened",
            "synchronize",
            "reopened",
            "ready_for_review",
            "converted_to_draft",
            "labeled",
            "unlabeled",
        ):
            self.assertIn(f"      - {action}\n", events)


class RequiredCheckTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        block = WORKFLOW.read_text().split(
            "      - name: Check selected job results\n", 1
        )[1]
        cls.script = textwrap.dedent(block.split("        run: |\n", 1)[1])

    def accepts(self, expected, results):
        return (
            subprocess.run(
                ["bash", "-e", "-c", self.script],
                env={
                    **os.environ,
                    "EXPECTED": json.dumps(expected),
                    "RESULTS": json.dumps(results),
                },
                capture_output=True,
                check=False,
            ).returncode
            == 0
        )

    def test_selected_jobs_must_succeed_and_only_planned_skips_pass(self):
        for event in (
            pr(True),
            pr(),
            pr(True, ("ci:full",)),
            pr(True, tuple(PLATFORM_GROUPS)),
        ):
            expected = plan("pull_request", event)["expected"]
            results = {job: {"result": wanted} for job, wanted in expected.items()}
            self.assertTrue(self.accepts(expected, results))
            for job, wanted in expected.items():
                for outcome in ("success", "skipped", "failure", "cancelled"):
                    if outcome != wanted:
                        with self.subTest(job=job, wanted=wanted, outcome=outcome):
                            self.assertFalse(
                                self.accepts(
                                    expected, {**results, job: {"result": outcome}}
                                )
                            )
                self.assertFalse(
                    self.accepts(
                        expected, {k: v for k, v in results.items() if k != job}
                    )
                )
            self.assertFalse(
                self.accepts(expected, {**results, "unexpected": {"result": "success"}})
            )
            self.assertFalse(self.accepts({}, results))
        self.assertFalse(self.accepts({}, {}))
        self.assertFalse(
            self.accepts({"plan": "skipped"}, {"plan": {"result": "skipped"}})
        )
