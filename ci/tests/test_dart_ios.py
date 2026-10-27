import unittest

from ci.generate_workflow import ROOT
from ci.workflow import load_configuration
from tools.ci.suites import target_suites


class DartIosTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        _, cls.presets = load_configuration(ROOT)

    def test_dart_ios_runs_on_the_simulator_and_builds_for_the_device(self):
        for preset, action in (
            ("ios-simulator-arm64-metal", "test:ios"),
            ("ios-arm64-metal", "build:mobile"),
        ):
            commands = [suite.command for suite in target_suites(self.presets, preset)]
            self.assertIn(
                ("mise", "run", f"//bindings/dart:{action}", preset), commands
            )
