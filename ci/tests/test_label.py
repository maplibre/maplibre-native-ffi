"""Exercise which labels the labeler adds."""

from __future__ import annotations

import json
import pathlib
import tempfile
import unittest
from unittest.mock import patch

from ci import label

CONFIG = label.load_config()
NAMES = [name for group in CONFIG["groups"].values() for name in group["labels"]]


def scores(overrides: dict[str, float]) -> dict[str, float]:
    return dict.fromkeys(NAMES, 0.1) | overrides


def added(decisions: dict) -> set[str]:
    return {name for name, entry in decisions.items() if entry["add"]}


class LabelTest(unittest.TestCase):
    def test_labels_go_only_into_groups_without_one(self):
        answers = scores(
            {
                "feature": 0.8,
                "area:render": 0.9,
                "area:map": 0.9,
                "binding:rust": 0.95,
                "platform:linux": 0.85,
                "renderer:vulkan": 0.5,
            }
        )
        decisions = label.decide(CONFIG, answers, {"bug", "area:map", "ci:full"})
        self.assertEqual(added(decisions), {"binding:rust", "platform:linux"})
        self.assertEqual(decisions["area:render"]["rule"], "group labeled")

    def test_a_required_group_gets_its_most_likely_label(self):
        answers = scores(
            {"chore": 0.4, "research": 0.35, "area:tooling": 0.6, "area:docs": 0.3}
        )
        decisions = label.decide(CONFIG, answers, set())
        self.assertEqual(added(decisions), {"chore", "area:tooling"})
        self.assertEqual(decisions["area:tooling"]["rule"], "most likely")

    def test_labels_set_while_jev_judged_win(self):
        def github(path):
            return [{"name": "bug"}] if path.endswith("/labels") else {}

        def post(args, **kwargs):
            posted.append(json.loads(kwargs["input"]))

        posted = []
        with tempfile.TemporaryDirectory() as directory:
            event = pathlib.Path(directory) / "event"
            event.write_text(json.dumps({"issue": {"number": 9}}))
            env = {
                "GITHUB_EVENT_PATH": str(event),
                "GITHUB_REPOSITORY": "owner/repo",
                "GITHUB_STEP_SUMMARY": str(pathlib.Path(directory) / "summary"),
                "TYPESAFE_API_KEY": "key",
            }
            judged = {"labels": scores({"feature": 0.9, "area:map": 0.9})}
            with (
                patch.dict("os.environ", env, clear=True),
                patch.object(label, "github", github),
                patch.object(label, "describe", return_value={}),
                patch.object(label, "judge", return_value=judged),
                patch.object(label.subprocess, "run", post),
                patch("builtins.print"),
            ):
                label.main()
        self.assertEqual(posted, [{"labels": ["area:map"]}])


if __name__ == "__main__":
    unittest.main()
