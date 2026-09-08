"""Exercise coverage transitions, generated job dependencies, and required checks."""

from __future__ import annotations

import copy
import itertools
import json
import os
import pathlib
import subprocess
import tempfile
import unittest
from unittest.mock import Mock, patch

from ruamel.yaml import YAML

from ci.coverage import (
    GROUPS,
    PLATFORMS,
    check_name,
    group_targets,
    scope,
    selected_jobs,
    workflow_file,
)
from ci.generate_workflow import ROOT, caller, serialize, suite, workflows
from ci.plan import plan
from ci.retry import main as retry_main
from ci.retry import retryable
from ci.workflow import load_configuration, preset_sets

ENV = {
    "GITHUB_REPOSITORY": "maplibre/maplibre-native-ffi",
    "GITHUB_SHA": "a" * 40,
    "GITHUB_RUN_ID": "100",
    "GITHUB_RUN_ATTEMPT": "1",
}


def pr(draft=False, labels=(), author="contributor", action="synchronize"):
    return {
        "action": action,
        "sender": {"login": "maintainer"},
        "pull_request": {
            "draft": draft,
            "labels": [{"name": label} for label in labels],
            "user": {"login": author},
            "head": {"sha": "b" * 40},
        },
    }


def api_run(run_id=90, head="b" * 40, event="pull_request"):
    return {"id": run_id, "head_sha": head, "event": event}


def proof(
    result="success", sha=ENV["GITHUB_SHA"], status="completed", selection="ready"
):
    return {
        "name": f"coverage / verified ({sha} / {selection})",
        "status": status,
        "conclusion": result,
        "html_url": "https://github.com/maplibre/maplibre-native-ffi/actions/runs/90/job/1",
    }


def fake_api(runs=None, jobs=None):
    return Mock(
        side_effect=[
            {"workflow_runs": [api_run()] if runs is None else runs},
            {
                "total_count": len(jobs or [proof()]),
                "jobs": [proof()] if jobs is None else jobs,
            },
        ]
    )


def shell_accepts(step, env):
    return (
        subprocess.run(
            ["bash", "-e", "-c", step["run"]],
            env={**os.environ, **env},
            capture_output=True,
            check=False,
        ).returncode
        == 0
    )


class CoverageTest(unittest.TestCase):
    def test_extended_combinations_include_packaging_producers_once(self):
        source, presets = load_configuration(ROOT)
        for size in range(len(PLATFORMS) + 1):
            for platforms in itertools.combinations(PLATFORMS, size):
                event = pr(labels=[f"ci:{name}" for name in platforms])
                selection = scope("extended", "pull_request", event)
                jobs = selected_jobs(source, presets, "extended", selection)
                self.assertEqual(len(jobs), len(set(jobs)))
                self.assertEqual("android-multi" in jobs, "android" in platforms)
                if "android" in platforms:
                    self.assertTrue(
                        {
                            f"target-android-{abi}-{backend}"
                            for abi in ("arm", "arm64", "x64")
                            for backend in ("egl", "vulkan")
                        }
                        <= set(jobs)
                    )
                self.assertNotIn("kotlin-maven", jobs)
        jobs = selected_jobs(source, presets, "extended", "full")
        self.assertTrue(
            {"hygiene", "docs", "android-multi", "kotlin-maven"} <= set(jobs)
        )
        self.assertTrue(
            {f"target-{target}" for target in preset_sets(presets)[0]} <= set(jobs)
        )

    def test_extended_reuse_requires_identical_complete_scope(self):
        for old_scope, labels, expected in (
            ("apple", ["ci:apple", "unrelated"], "reuse"),
            ("apple", ["ci:apple", "ci:android"], "run"),
            ("android+apple", ["ci:apple"], "run"),
            ("apple", ["ci:full"], "run"),
            ("full", ["ci:full", "ci:android"], "reuse"),
            ("android+apple", ["ci:apple", "ci:android"], "reuse"),
        ):
            with self.subTest(old_scope=old_scope, labels=labels):
                event = pr(labels=labels, action="labeled")
                api = fake_api(jobs=[proof(selection=old_scope)])
                self.assertEqual(
                    plan("extended", "pull_request", event, ENV, api)["mode"], expected
                )

    def test_readiness_labels_and_authorship_select_complete_groups(self):
        for draft, full_label, dependabot in itertools.product((False, True), repeat=3):
            for size in range(len(PLATFORMS) + 1):
                for groups in itertools.combinations(PLATFORMS, size):
                    labels = [f"ci:{group}" for group in groups]
                    if full_label:
                        labels.append("ci:full")
                    event = pr(
                        draft,
                        labels,
                        "dependabot[bot]" if dependabot else "contributor",
                    )
                    selected = {
                        group for group in GROUPS if scope(group, "pull_request", event)
                    }
                    expected = {"baseline"}
                    if not draft:
                        expected.add("ready")
                    if full_label or dependabot or groups:
                        expected.add("extended")
                    self.assertEqual(selected, expected)
        for event_name in ("push", "workflow_dispatch"):
            self.assertEqual(plan("extended", event_name, {}, ENV)["mode"], "run")
        event = pr(True)
        event["sender"]["login"] = "dependabot[bot]"
        self.assertFalse(scope("extended", "pull_request", event))
        for invalid in ({}, {"pull_request": {}}, {"pull_request": {"labels": []}}):
            with self.assertRaises(KeyError):
                scope("extended", "pull_request", invalid)
        with self.assertRaises(ValueError):
            scope("extended", "unknown", {})

    def test_baseline_ready_and_full_cover_the_original_targets(self):
        source, presets = load_configuration(ROOT)
        self.assertEqual(
            group_targets(source, presets, "baseline"),
            {
                "linux-gnu-x64-egl",
                "linux-gnu-x64-vulkan",
            },
        )
        self.assertEqual(
            group_targets(source, presets, "ready"),
            {
                "macos-arm64-metal",
                "windows-x64-wgl",
                "windows-x64-vulkan",
                "android-x64-egl",
                "android-x64-vulkan",
                "emscripten-wasm32-webgl",
                "emscripten-wasm32-webgpu",
            },
        )
        self.assertEqual(
            group_targets(source, presets, "extended"), set(preset_sets(presets)[0])
        )
        for invalid in ([], ["not-a-preset"], source["coverage"]["baseline"]):
            changed = copy.deepcopy(source)
            changed["coverage"]["ready"] = invalid
            with self.assertRaises(ValueError):
                group_targets(changed, presets, "extended")

    def test_label_and_readiness_transitions_preserve_actual_coverage(self):
        for group, event in (
            ("ready", pr(action="ready_for_review")),
            ("extended", pr(True, ["ci:apple", "unrelated"], action="labeled")),
            ("extended", pr(True, ["ci:full"], action="labeled")),
        ):
            for result in ("success", "failure"):
                api = fake_api(
                    jobs=[proof(result, selection=scope(group, "pull_request", event))]
                )
                selection = plan(group, "pull_request", event, ENV, api)
                self.assertEqual(selection["mode"], "reuse")
                self.assertEqual(selection["result"], result)
                self.assertIn(workflow_file(group), api.call_args_list[0].args[0])
                self.assertIn("filter=latest", api.call_args_list[1].args[0])
        api = Mock(
            side_effect=AssertionError("Omitted coverage must not query history")
        )
        for group, event in (
            ("ready", pr(True, action="converted_to_draft")),
            ("extended", pr(labels=[], action="unlabeled")),
            ("extended", pr(labels=["unrelated"], action="unlabeled")),
        ):
            self.assertEqual(
                plan(group, "pull_request", event, ENV, api)["mode"], "omit"
            )

    def test_missing_cancelled_or_different_commit_evidence_executes_coverage(self):
        event = pr(True, ["ci:apple"], action="labeled")
        for jobs in (
            [],
            [proof(sha="c" * 40, selection="apple")],
            [proof("cancelled", selection="apple")],
            [proof(None, status="in_progress", selection="apple")],
            [{**proof(), "name": "ci-required"}],
        ):
            with self.subTest(jobs=jobs):
                self.assertEqual(
                    plan("extended", "pull_request", event, ENV, fake_api(jobs=jobs))[
                        "mode"
                    ],
                    "run",
                )
        # Current runs, unrelated workflow events, and another PR head cannot
        # supply evidence even if the service returns them in the filtered list.
        for candidate in (api_run(100), api_run(head="c" * 40), api_run(event="push")):
            api = Mock(return_value={"workflow_runs": [candidate]})
            self.assertEqual(
                plan("extended", "pull_request", event, ENV, api)["mode"], "run"
            )
            self.assertEqual(api.call_count, 1)
        with patch("builtins.print"):
            api = Mock(side_effect=subprocess.CalledProcessError(1, "gh"))
            self.assertEqual(
                plan("extended", "pull_request", event, ENV, api)["mode"], "run"
            )

    def test_code_updates_reopens_and_explicit_reruns_execute_again(self):
        api = Mock(side_effect=AssertionError("Fresh coverage must not query history"))
        for action in ("opened", "synchronize", "reopened", "labeled"):
            event = pr(True, ["ci:full"], action=action)
            env = {**ENV, "GITHUB_RUN_ATTEMPT": "2"} if action == "labeled" else ENV
            self.assertEqual(
                plan("extended", "pull_request", event, env, api)["mode"], "run"
            )

    def test_latest_actual_failure_cannot_be_hidden_by_an_older_success(self):
        api = fake_api(runs=[api_run(80), api_run(90)], jobs=[proof("failure")])
        selection = plan(
            "ready", "pull_request", pr(action="ready_for_review"), ENV, api
        )
        self.assertEqual(selection["result"], "failure")
        self.assertIn("/runs/90/", api.call_args.args[0])
        # A later omitted/restated run is not evidence of executed coverage.
        api = Mock(
            side_effect=[
                {"workflow_runs": [api_run(90), api_run(80)]},
                {
                    "total_count": 1,
                    "jobs": [{**proof(), "name": "ci-required (ready)"}],
                },
                {"total_count": 1, "jobs": [proof("failure")]},
            ]
        )
        self.assertEqual(
            plan("ready", "pull_request", pr(action="ready_for_review"), ENV, api)[
                "result"
            ],
            "failure",
        )

    def test_entrypoint_runs_outside_checkout_without_project_dependencies(self):
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            (root / "event").write_text(json.dumps(pr(True)))
            subprocess.run(
                ["bash", str(ROOT / ".mise/tasks/ci/plan")],
                cwd=root,
                check=True,
                env={
                    "PATH": os.environ["PATH"],
                    **ENV,
                    "CI_GROUP": "baseline",
                    "GITHUB_EVENT_NAME": "pull_request",
                    "GITHUB_EVENT_PATH": str(root / "event"),
                    "GITHUB_OUTPUT": str(root / "output"),
                    "GITHUB_STEP_SUMMARY": str(root / "summary"),
                },
            )
            self.assertIn("mode=run", (root / "output").read_text())


class WorkflowTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source, cls.presets = load_configuration(ROOT)
        cls.workflows = workflows(cls.source, cls.presets)

    def test_generated_workflows_match_and_preserve_packaging_dependencies(self):
        self.assertEqual(
            {
                document["jobs"]["required"]["name"]
                for document in self.workflows.values()
                if "required" in document["jobs"]
            },
            {"ci-required", "ci-required (baseline)", "ci-required (ready)"},
        )
        for name, document in self.workflows.items():
            with self.subTest(name=name):
                encoded = serialize(document)
                self.assertEqual(
                    (ROOT / ".github/workflows" / name).read_text(), encoded
                )
                self.assertEqual(YAML(typ="safe").load(encoded), document)
        for group in GROUPS:
            jobs = self.workflows[f"_ci-{group}.yml"]["jobs"]
            self.assertEqual(set(jobs["verified"]["needs"]), set(jobs) - {"verified"})
            for job in jobs.values():
                self.assertNotIn("continue-on-error", job)
                self.assertTrue(set(job.get("needs", [])) <= jobs.keys())
            if group == "extended":
                self.assertEqual(
                    set(jobs["android-multi"]["needs"]),
                    {
                        f"target-android-{abi}-{backend}"
                        for abi in ("arm", "arm64", "x64")
                        for backend in ("egl", "vulkan")
                    },
                )
            self.assertEqual("kotlin-maven" in jobs, group == "extended")
        full = self.workflows["_ci-extended.yml"]["jobs"]
        self.assertEqual(full["kotlin-maven"]["with"]["run_id"], "${{ github.run_id }}")
        self.assertEqual(full["kotlin-maven"]["with"]["sha"], "${{ github.sha }}")
        self.assertFalse(full["kotlin-maven"]["with"]["publish"])
        self.assertEqual(self.workflows["ci.yml"]["name"], "CI")

    def test_events_keep_baseline_stable_and_main_runs_independent(self):
        baseline = caller("baseline")["on"]["pull_request"]["types"]
        self.assertEqual(set(baseline), {"opened", "synchronize", "reopened"})
        ready = caller("ready")["on"]["pull_request"]["types"]
        self.assertEqual(
            set(ready) - set(baseline), {"ready_for_review", "converted_to_draft"}
        )
        for group in GROUPS:
            document = caller(group)
            self.assertEqual("push" in document["on"], group == "extended")
            self.assertEqual("workflow_dispatch" in document["on"], group == "extended")
            self.assertIn("|| github.run_id", document["concurrency"]["group"])
            self.assertEqual(
                document["concurrency"]["cancel-in-progress"],
                "${{ github.event.action == 'synchronize' }}",
            )
            self.assertEqual(document["jobs"]["required"]["name"], check_name(group))

    def test_verification_rejects_every_missing_failed_cancelled_or_skipped_job(self):
        job = suite(self.source, self.presets, "extended")["jobs"]["verified"]
        step = job["steps"][0]
        expected = json.loads(step["env"]["EXPECTED"])
        results = {name: {"result": "success"} for name in expected}

        def accepts(values):
            return shell_accepts(
                step,
                {
                    "EXPECTED": json.dumps(expected),
                    "RESULTS": json.dumps(values),
                    "SELECTED": json.dumps(expected),
                },
            )

        self.assertTrue(accepts(results))
        for name in expected:
            for result in ("failure", "cancelled", "skipped"):
                self.assertFalse(
                    accepts({**results, name: {"result": result}}), (name, result)
                )
            self.assertFalse(
                accepts({key: value for key, value in results.items() if key != name})
            )
        self.assertFalse(accepts({**results, "unexpected": {"result": "success"}}))

    def test_extended_gate_accepts_only_skips_outside_selected_scope(self):
        step = suite(self.source, self.presets, "extended")["jobs"]["verified"][
            "steps"
        ][0]
        expected = json.loads(step["env"]["EXPECTED"])
        selected = selected_jobs(self.source, self.presets, "extended", "android+apple")
        results = {
            name: {"result": "success" if name in selected else "skipped"}
            for name in expected
        }

        def accepts(values, selection=selected):
            return shell_accepts(
                step,
                {
                    "EXPECTED": json.dumps(expected),
                    "SELECTED": json.dumps(selection),
                    "RESULTS": json.dumps(values),
                },
            )

        self.assertTrue(accepts(results))
        self.assertFalse(accepts(results, []))
        self.assertFalse(accepts(results, [*selected, "unknown"]))
        for name in expected:
            for result in {"success", "failure", "skipped", "cancelled"} - {
                results[name]["result"]
            }:
                self.assertFalse(
                    accepts({**results, name: {"result": result}}), (name, result)
                )

    def test_required_gate_only_accepts_executed_success_reused_success_or_omission(
        self,
    ):
        step = caller("extended")["jobs"]["required"]["steps"][0]
        for plan_result, mode, previous, result in itertools.product(
            ("success", "failure", "cancelled", "skipped"),
            ("run", "reuse", "omit", ""),
            ("success", "failure", ""),
            ("success", "failure", "cancelled", "skipped"),
        ):
            expected = plan_result == "success" and (
                (mode == "run" and result == "success")
                or (mode == "reuse" and result == "skipped" and previous == "success")
                or (mode == "omit" and result == "skipped")
            )
            self.assertEqual(
                shell_accepts(
                    step,
                    {
                        "PLAN": plan_result,
                        "MODE": mode,
                        "PREVIOUS": previous,
                        "RESULT": result,
                    },
                ),
                expected,
                (plan_result, mode, previous, result),
            )


class RetryTest(unittest.TestCase):
    def test_api_failures_retry_with_backoff_and_recheck_run_attempt(self):
        jobs = {
            "jobs": [
                {"name": "plan", "conclusion": "failure"},
                {"name": "ci-required", "conclusion": "failure"},
            ]
        }
        error = subprocess.CalledProcessError(1, "gh")
        for failure_at in ("run", "jobs", "post"):
            responses = {
                "run": [error, '{"run_attempt": 1}', json.dumps([jobs])],
                "jobs": [
                    '{"run_attempt": 1}',
                    error,
                    '{"run_attempt": 1}',
                    json.dumps([jobs]),
                ],
                "post": [
                    '{"run_attempt": 1}',
                    json.dumps([jobs]),
                    '{"run_attempt": 2}',
                ],
            }[failure_at]
            with (
                self.subTest(failure_at=failure_at),
                patch.dict(
                    os.environ,
                    {"CI_RUN_ID": "90", "GITHUB_REPOSITORY": ENV["GITHUB_REPOSITORY"]},
                ),
                patch("ci.retry.subprocess.check_output", side_effect=responses),
                patch(
                    "ci.retry.subprocess.run",
                    side_effect=error if failure_at == "post" else None,
                ) as post,
                patch("ci.retry.time.sleep") as sleep,
                patch("builtins.print"),
            ):
                retry_main()
                sleep.assert_called_once_with(2)
                self.assertEqual(post.call_count, 1)
        with (
            patch.dict(
                os.environ,
                {"CI_RUN_ID": "90", "GITHUB_REPOSITORY": ENV["GITHUB_REPOSITORY"]},
            ),
            patch("ci.retry.subprocess.check_output", side_effect=error) as get,
            patch("ci.retry.time.sleep") as sleep,
            patch("builtins.print"),
        ):
            with self.assertRaises(subprocess.CalledProcessError):
                retry_main()
            self.assertEqual(get.call_count, 4)
            self.assertEqual([call.args[0] for call in sleep.call_args_list], [2, 4, 8])

    def test_one_primary_failure_retries_with_both_aggregates(self):
        jobs = [
            {"name": "coverage / target / linux-gnu-x64-egl", "conclusion": "failure"},
            {"name": "coverage / verified (sha)", "conclusion": "failure"},
            {"name": "ci-required", "conclusion": "failure"},
            {"name": "coverage / Kotlin Maven / verify", "conclusion": "skipped"},
        ]
        self.assertTrue(retryable(jobs))
        self.assertFalse(retryable(jobs[1:]))
        self.assertFalse(retryable(jobs + [{"name": "other", "conclusion": "failure"}]))
        self.assertFalse(
            retryable(jobs + [{"name": "other", "conclusion": "cancelled"}])
        )
        self.assertTrue(
            retryable(
                [
                    {"name": "plan", "conclusion": "failure"},
                    {"name": "ci-required (ready)", "conclusion": "failure"},
                ]
            )
        )
