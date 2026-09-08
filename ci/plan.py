"""Execute required coverage or reuse its verdict for the exact tested commit."""

from __future__ import annotations

import json
import os
import pathlib
import subprocess

from ci.coverage import STATE_EVENTS, scope, selected_jobs, workflow_file
from ci.workflow import load_configuration


def github(path: str) -> dict:
    return json.loads(subprocess.check_output(["gh", "api", path], text=True))


def previous_verdict(
    group: str, selection: str, event: dict, env: dict, api=github
) -> dict | None:
    repository = env["GITHUB_REPOSITORY"]
    prefix = f"repos/{repository}/actions"
    head = event["pull_request"]["head"]["sha"]
    # The marker exists only after actual coverage, never after an omitted or
    # restated run. Match both the tested merge commit and the complete scope.
    # The merge SHA also identifies the workflow definitions for that scope.
    marker = f"coverage / verified ({env['GITHUB_SHA']} / {selection})"
    runs = api(
        f"{prefix}/workflows/{workflow_file(group)}/runs"
        f"?event=pull_request&head_sha={head}&per_page=100"
    )["workflow_runs"]
    for run in sorted(runs, key=lambda run: run["id"], reverse=True):
        if str(run["id"]) == env["GITHUB_RUN_ID"]:
            continue
        if run["head_sha"] != head or run["event"] != "pull_request":
            continue
        # Each workflow has fewer than 100 jobs, including Maven's nested
        # jobs. Request the current attempt so a retry replaces its old verdict.
        response = api(f"{prefix}/runs/{run['id']}/jobs?filter=latest&per_page=100")
        if response["total_count"] > 100:
            raise ValueError("Coverage jobs exceed the verdict lookup page")
        matches = [job for job in response["jobs"] if job["name"] == marker]
        if not matches:
            continue
        if len(matches) != 1:
            raise ValueError("Duplicate coverage verdicts")
        job = matches[0]
        if job["status"] != "completed" or job["conclusion"] not in {
            "success",
            "failure",
        }:
            return None
        return {"result": job["conclusion"], "url": job["html_url"]}
    # A bounded lookup is an optimization. Missing evidence executes coverage.
    return None


def plan(group: str, event_name: str, event: dict, env: dict, api=github) -> dict:
    selection = scope(group, event_name, event)
    if not selection:
        return {"mode": "omit", "result": "", "url": ""}
    if (
        event_name == "pull_request"
        and event["action"] in STATE_EVENTS
        and env["GITHUB_RUN_ATTEMPT"] == "1"
    ):
        try:
            verdict = previous_verdict(group, selection, event, env, api)
        except subprocess.CalledProcessError:
            # A new workflow can lack a registered API endpoint. An unavailable
            # lookup loses the optimization, never the required coverage.
            print("Could not retrieve previous coverage; executing it again.")
            verdict = None
        if verdict is not None:
            return {"mode": "reuse", **verdict}
    return {"mode": "run", "result": "", "url": ""}


def main() -> None:
    env = dict(os.environ)
    group = env["CI_GROUP"]
    event = json.loads(pathlib.Path(env["GITHUB_EVENT_PATH"]).read_text())
    selection = plan(
        group,
        env["GITHUB_EVENT_NAME"],
        event,
        env,
    )
    selection["scope"] = scope(
        group,
        env["GITHUB_EVENT_NAME"],
        event,
    )
    source, presets = load_configuration(pathlib.Path(__file__).resolve().parents[1])
    selection["jobs"] = json.dumps(
        selected_jobs(source, presets, group, selection["scope"])
    )
    with pathlib.Path(env["GITHUB_OUTPUT"]).open("a") as output:
        for key, value in selection.items():
            print(f"{key}={value}", file=output)
    with pathlib.Path(env["GITHUB_STEP_SUMMARY"]).open("a") as summary:
        print(f"CI **{group}**: {selection['mode']}.", file=summary)
        if selection["mode"] == "reuse":
            print(
                f"\nPrevious coverage: {selection['result']} — {selection['url']}",
                file=summary,
            )


if __name__ == "__main__":
    main()
