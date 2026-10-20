"""Select the coverage a CI run needs.

Main pushes and manual runs execute every unit. A pull request push asks Jev
which units the change puts at risk, judging each unit only on what changed
since that unit last passed for the pull request.
"""

from __future__ import annotations

import json
import os
import pathlib
import subprocess
import time
import urllib.parse

from ci import select
from ci.coverage import HYGIENE, dependencies, selected_jobs, unit_jobs, unit_passed
from ci.workflow import load_configuration

ROOT = pathlib.Path(__file__).resolve().parents[1]
WORKFLOW = "ci.yml"
# Leave room within the plan job's timeout to fall back to every unit.
SELECTION_SECONDS = 300
LOOKBACK_RUNS = 30


def github(path: str) -> dict:
    return json.loads(subprocess.check_output(["gh", "api", path], text=True))


def present(sha: str) -> bool:
    """Make a commit from an earlier run available, if it still exists."""
    exists = ["git", "cat-file", "-e", f"{sha}^{{commit}}"]
    if (
        subprocess.run(exists, cwd=ROOT, capture_output=True, check=False).returncode
        == 0
    ):
        return True
    fetch = [
        "git",
        "fetch",
        "--quiet",
        "--no-tags",
        "--filter=blob:none",
        "origin",
        sha,
    ]
    return (
        subprocess.run(fetch, cwd=ROOT, capture_output=True, check=False).returncode
        == 0
    )


def last_passed(
    owned: dict[str, list[str]], event: dict, env: dict, api=github
) -> dict[str, str]:
    """Return the latest pull request head at which each unit passed."""
    repository = env["GITHUB_REPOSITORY"]
    pr = event["pull_request"]
    branch = urllib.parse.quote(pr["head"]["ref"], safe="")
    runs = api(
        f"repos/{repository}/actions/workflows/{WORKFLOW}/runs"
        f"?event=pull_request&branch={branch}&per_page={LOOKBACK_RUNS}"
    )["workflow_runs"]
    passed: dict[str, str] = {}
    for run in sorted(runs, key=lambda run: run["id"], reverse=True):
        if str(run["id"]) == env["GITHUB_RUN_ID"] or not any(
            other["number"] == pr["number"] for other in run.get("pull_requests") or []
        ):
            continue
        # The current attempt replaces earlier ones, so a retried failure
        # counts only by its retry.
        response = api(
            f"repos/{repository}/actions/runs/{run['id']}/jobs?filter=latest&per_page=100"
        )
        if response["total_count"] > 100:
            raise ValueError("CI jobs exceed the lookup page")
        conclusions = {job["name"]: job["conclusion"] for job in response["jobs"]}
        for unit, jobs in owned.items():
            if unit not in passed and unit_passed(jobs, conclusions):
                passed[unit] = run["head_sha"]
        if len(passed) == len(owned):
            break
    return {unit: sha for unit, sha in passed.items() if present(sha)}


def jev_choice(config: dict, api_key: str, deadline: float):
    def choose(base: str, head: str, message: str) -> dict[str, tuple[bool, str]]:
        result = select.select(
            base, head, message, config=config, api_key=api_key, deadline=deadline
        )
        if result.get("oversized"):
            return dict.fromkeys(result["units"], (True, "change too large to judge"))
        forced = select.forced_units(config, select.changed_paths(base, head))
        return {
            unit: (True, "changed path")
            if unit in forced
            else (p >= select.THRESHOLD, f"Jev {p:.2f}")
            for unit, p in result["units"].items()
        }

    return choose


def everything(config: dict, reason: str) -> dict[str, tuple[bool, str]]:
    return {
        unit: (True, reason) for unit in [HYGIENE, *(u["id"] for u in config["units"])]
    }


def plan(
    config: dict,
    owned: dict[str, list[str]],
    event_name: str,
    event: dict,
    env: dict,
    *,
    api=github,
    choose=None,
) -> dict[str, tuple[bool, str]]:
    """Return whether each unit runs, and why."""
    if event_name in {"push", "workflow_dispatch"}:
        return everything(config, "full coverage outside pull requests")
    if event_name != "pull_request":
        raise ValueError(f"Unsupported CI event: {event_name}")
    pr = event["pull_request"]
    labels = {label["name"] for label in pr["labels"]}
    if "ci:full" in labels:
        return everything(config, "label ci:full")
    if not env.get("TYPESAFE_API_KEY") and choose is None:
        return everything(
            config, "no Jev API key, as in fork and Dependabot pull requests"
        )
    choose = choose or jev_choice(
        config, env["TYPESAFE_API_KEY"], time.monotonic() + SELECTION_SECONDS
    )

    try:
        head = pr["head"]["sha"]
        fork = select.git("merge-base", pr["base"]["sha"], head).strip()
        passed = last_passed(owned, event, env, api)
        bases: dict[str, list[str]] = {}
        for unit in owned:
            bases.setdefault(passed.get(unit, fork), []).append(unit)
        decisions = {}
        for base, units in bases.items():
            since = "since the merge base" if base == fork else f"since {base[:9]}"
            if base == head:
                decisions |= {unit: (False, f"passed at {head[:9]}") for unit in units}
                continue
            if units != [HYGIENE]:
                message = f"{pr['title']}\n\n{select.git('log', '--format=%B', f'{base}..{head}')[:4000]}"
                answers = choose(base, head, message)
            for unit in units:
                if unit == HYGIENE:
                    decisions[unit] = (True, f"changed {since}")
                else:
                    selected, reason = answers[unit]
                    decisions[unit] = (selected, f"{reason} {since}")
    # Selection is an optimization, so any failure runs every unit.
    except Exception as error:  # noqa: BLE001
        return everything(config, f"selection failed: {error}")

    for unit in owned:
        if f"ci:{unit}" in labels:
            decisions[unit] = (True, f"label ci:{unit}")
    return decisions


def main() -> None:
    env = dict(os.environ)
    event = json.loads(pathlib.Path(env["GITHUB_EVENT_PATH"]).read_text())
    config = select.load_config()
    source, presets = load_configuration(ROOT)
    owned = unit_jobs(config, source, presets)
    decisions = plan(config, owned, env["GITHUB_EVENT_NAME"], event, env)
    units = [unit for unit, (selected, _) in decisions.items() if selected]
    jobs = selected_jobs(owned, dependencies(source, presets), units)
    with pathlib.Path(env["GITHUB_OUTPUT"]).open("a") as output:
        print(f"jobs={json.dumps(jobs)}", file=output)
    with pathlib.Path(env["GITHUB_STEP_SUMMARY"]).open("a") as summary:
        print("| Unit | Runs | Reason |\n| --- | --- | --- |", file=summary)
        for unit, (selected, reason) in decisions.items():
            print(
                f"| {unit} | {'yes' if selected else 'no'} | {reason} |", file=summary
            )


if __name__ == "__main__":
    main()
