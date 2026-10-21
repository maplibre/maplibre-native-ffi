"""Exercise the coverage units, the required check, and retries."""

from __future__ import annotations

import json
import os
import re
import subprocess
import unittest

from ruamel.yaml import YAML

from ci.coverage import HYGIENE, unit_jobs
from ci.generate_workflow import ROOT, ci_workflow
from ci.retry import retryable
from ci.workflow import load_configuration

ALL = "fromJSON(needs.plan.outputs.all)"
UNIT = re.compile(r"contains\(fromJSON\(needs\.plan\.outputs\.units\), '([^']+)'\)")


class CoverageTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source, cls.presets = load_configuration(ROOT)
        cls.jobs = ci_workflow(cls.source, cls.presets)["jobs"]

    def test_units_match_the_rubberstamp_config(self):
        config = YAML(typ="safe").load(ROOT / ".config/rubberstamp.yml")
        self.assertEqual(sorted(self.source["units"]), sorted(config["ci"]["units"]))

    def units(self, job: dict) -> set[str]:
        """Return the units that a job's condition runs it for."""
        units = UNIT.findall(job["if"])
        clauses = [
            f"contains(fromJSON(needs.plan.outputs.units), '{u}')" for u in units
        ]
        self.assertEqual(job["if"], " || ".join([ALL, *clauses]))
        return set(units)

    def test_each_unit_runs_its_jobs_and_what_they_consume(self):
        gated = {
            name: job
            for name, job in self.jobs.items()
            if name not in {"plan", HYGIENE, "required"}
        }
        owned = unit_jobs(self.source, self.presets)
        for unit, jobs in owned.items():
            with self.subTest(unit=unit):
                running = {n for n, job in gated.items() if unit in self.units(job)}
                self.assertLessEqual(set(jobs), running)
                for name in running:
                    waits = set(gated[name]["needs"]) - {"plan"}
                    self.assertLessEqual(waits, running, name)


class GateTest(unittest.TestCase):
    def test_required_check_accepts_only_successes_and_skips(self):
        source, presets = load_configuration(ROOT)
        workflow = ci_workflow(source, presets)
        step = workflow["jobs"]["required"]["steps"][0]
        needs = workflow["jobs"]["required"]["needs"]
        results = {job: {"result": "success"} for job in needs}

        def accepts(values, plan_result="success"):
            env = {
                "PATH": os.environ["PATH"],
                "PLAN": plan_result,
                "RESULTS": json.dumps(values),
            }
            command = ["bash", "-c", step["run"]]
            done = subprocess.run(command, env=env, capture_output=True, check=False)
            return done.returncode == 0

        self.assertTrue(accepts(results))
        skipped = {job: {"result": "skipped"} for job in needs if job != "plan"}
        self.assertTrue(accepts({**results, **skipped}))
        self.assertFalse(accepts(results, plan_result="failure"))
        self.assertFalse(accepts(results, plan_result="skipped"))
        for job in needs:
            for result in ("failure", "cancelled"):
                changed = {**results, job: {"result": result}}
                self.assertFalse(accepts(changed), (job, result))


class RetryTest(unittest.TestCase):
    def test_only_one_primary_failure_with_the_required_check_retries(self):
        jobs = [
            {"name": "target / linux-gnu-x64-egl", "conclusion": "failure"},
            {"name": "ci-required", "conclusion": "failure"},
            {"name": "Kotlin Maven / verify", "conclusion": "skipped"},
        ]
        self.assertTrue(retryable(jobs))
        self.assertFalse(retryable(jobs[1:]))
        for other in ("failure", "cancelled"):
            self.assertFalse(retryable([*jobs, {"name": "other", "conclusion": other}]))
