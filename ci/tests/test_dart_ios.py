import unittest

from ci.generate_workflow import ROOT, ci_workflow
from ci.workflow import load_configuration


class DartIosTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source, cls.presets = load_configuration(ROOT)

    def test_dart_ios_runs_on_the_simulator_and_builds_for_the_device(self):
        jobs = ci_workflow(self.source, self.presets)["jobs"]
        for preset, action in (
            ("ios-simulator-arm64-metal", "test:ios"),
            ("ios-arm64-metal", "build:mobile"),
        ):
            commands = [step.get("run") for step in jobs[f"target-{preset}"]["steps"]]
            self.assertIn(f"mise run //bindings/dart:{action} {preset}", commands)
