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
from ci.coverage import HYGIENE, dependencies, selected_jobs, unit_jobs, unit_outcome
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


def history(
    owned: dict[str, list[str]], event: dict, env: dict, api=github
) -> tuple[dict[str, str], set[str]]:
    """Return each unit's latest passing head, and the units that last failed."""
    repository = env["GITHUB_REPOSITORY"]
    pr = event["pull_request"]
    branch = urllib.parse.quote(pr["head"]["ref"], safe="")
    runs = api(
        f"repos/{repository}/actions/workflows/{WORKFLOW}/runs"
        f"?event=pull_request&branch={branch}&per_page={LOOKBACK_RUNS}"
    )["workflow_runs"]
    passed: dict[str, str] = {}
    failed: set[str] = set()
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
            if unit in passed or unit in failed:
                continue
            outcome = unit_outcome(jobs, conclusions)
            if outcome == "passed":
                passed[unit] = run["head_sha"]
            elif outcome == "failed":
                failed.add(unit)
        if len(passed) + len(failed) == len(owned):
            break
    return {unit: sha for unit, sha in passed.items() if present(sha)}, failed


def decision(run: bool, *, score: float | None = None, rule: str = "", **since) -> dict:
    """Describe one unit's decision: Jev's score, a rule, or both."""
    return {"run": run, "score": score, "rule": rule, **since}


def jev_choice(
    config: dict,
    api_key: str,
    deadline: float | None = None,
    *,
    model: str = select.DEFAULT_MODEL,
    cache: pathlib.Path | None = None,
    on_request=None,
):
    def choose(base: str, head: str, message: str) -> dict[str, dict]:
        result = select.select(
            base,
            head,
            message,
            config=config,
            model=model,
            api_key=api_key,
            deadline=deadline,
            cache=cache,
            on_request=on_request,
        )
        if result.get("oversized"):
            return dict.fromkeys(
                result["units"], decision(True, rule="too large to judge")
            )
        forced = select.forced_units(config, select.changed_paths(base, head))
        return {
            unit: decision(
                unit in forced or p >= select.THRESHOLD,
                score=p,
                rule=f"matches {forced[unit]}" if unit in forced else "",
            )
            for unit, p in result["units"].items()
        }

    return choose


def everything(config: dict, rule: str) -> dict[str, dict]:
    units = [HYGIENE, *(unit["id"] for unit in config["units"])]
    return {unit: decision(True, rule=rule) for unit in units}


def plan(
    config: dict,
    owned: dict[str, list[str]],
    event_name: str,
    event: dict,
    env: dict,
    *,
    api=github,
    choose=None,
    log=lambda line: None,
) -> dict[str, dict]:
    """Return whether each unit runs, and the score or rule that decided it."""
    if event_name == "push":
        return everything(config, "full: main")
    if event_name == "workflow_dispatch":
        return everything(config, "full: manual run")
    if event_name != "pull_request":
        raise ValueError(f"Unsupported CI event: {event_name}")
    pr = event["pull_request"]
    labels = {label["name"] for label in pr["labels"]}
    if "ci:full" in labels:
        return everything(config, "full: label ci:full")
    if not env.get("TYPESAFE_API_KEY") and choose is None:
        return everything(config, "full: no Jev API key")
    choose = choose or jev_choice(
        config,
        env["TYPESAFE_API_KEY"],
        time.monotonic() + SELECTION_SECONDS,
        on_request=select.log_request,
    )

    try:
        head = pr["head"]["sha"]
        fork = select.git("merge-base", pr["base"]["sha"], head).strip()
        passed, failed = history(owned, event, env, api)
        bases: dict[str, list[str]] = {}
        for unit in owned:
            bases.setdefault(passed.get(unit, fork), []).append(unit)
        decisions = {}
        for base, units in bases.items():
            since = {"since": base, "merge_base": base == fork}
            if base == head:
                for unit in units:
                    decisions[unit] = decision(
                        False, rule="passed at this head", **since
                    )
                continue
            # Judge only the pull request's own changes, not main's merged ones.
            judged = base if base == fork else select.own_changes_base(base, fork)
            changed = bool(select.changed_paths(judged, head))
            if changed and units != [HYGIENE]:
                origin = "the merge base" if base == fork else "its last pass"
                log(
                    f"Judging the changes since {base[:9]}, {origin}, for {', '.join(units)}"
                )
                messages = select.git("log", "--format=%B", f"{base}..{head}")[:4000]
                answers = choose(judged, head, f"{pr['title']}\n\n{messages}")
            for unit in units:
                if unit == HYGIENE:
                    decisions[unit] = decision(True, rule="head changed", **since)
                elif not changed:
                    decisions[unit] = decision(False, rule="no new changes", **since)
                else:
                    decisions[unit] = answers[unit] | since
    # Selection is an optimization, so any failure runs every unit.
    except Exception as error:  # noqa: BLE001
        return everything(config, f"full: selection failed: {error}")

    # A unit runs until it passes again, whatever Jev answers for the new change.
    for unit in failed:
        decisions[unit] = decisions[unit] | {"run": True, "rule": "failed last run"}
    for unit in owned:
        if f"ci:{unit}" in labels:
            decisions[unit] = decisions[unit] | {
                "run": True,
                "rule": f"label ci:{unit}",
            }
    return decisions


def describe(entry: dict) -> str:
    score = f"{entry['score']:.2f}" if entry["score"] is not None else ""
    if entry["rule"] and score:
        return f"{entry['rule']} ({score})"
    return entry["rule"] or score


def main() -> None:
    env = dict(os.environ)
    event = json.loads(pathlib.Path(env["GITHUB_EVENT_PATH"]).read_text())
    config = select.load_config()
    source, presets = load_configuration(ROOT)
    owned = unit_jobs(config, source, presets)
    decisions = plan(config, owned, env["GITHUB_EVENT_NAME"], event, env, log=print)
    units = [unit for unit, entry in decisions.items() if entry["run"]]
    jobs = selected_jobs(owned, dependencies(source, presets), units)
    with pathlib.Path(env["GITHUB_OUTPUT"]).open("a") as output:
        print(f"jobs={json.dumps(jobs)}", file=output)

    heading = (
        f"{len(units)} of {len(decisions)} units run. Jev's score selects a unit "
        f"at {select.THRESHOLD:.2f} or above."
    )
    commits = f"{env.get('GITHUB_SERVER_URL', 'https://github.com')}/{env.get('GITHUB_REPOSITORY')}/commit"
    print(heading)
    rows = ["| | Unit | Score or rule | Judged since |", "| --- | --- | --- | --- |"]
    for unit, entry in decisions.items():
        mark = "✅" if entry["run"] else "➖"
        sha = entry.get("since", "")
        since = (
            f"{sha[:9]}{' (merge base)' if entry.get('merge_base') else ''}"
            if sha
            else ""
        )
        print(f"{mark} {unit:<13} {describe(entry):<28} {since}")
        link = f"[`{sha[:9]}`]({commits}/{sha})" if sha else ""
        base = f"{link} (merge base)" if entry.get("merge_base") else link
        rows.append(f"| {mark} | {unit} | {describe(entry)} | {base} |")
    with pathlib.Path(env["GITHUB_STEP_SUMMARY"]).open("a") as summary:
        print(f"{heading}\n\n" + "\n".join(rows), file=summary)


if __name__ == "__main__":
    main()
