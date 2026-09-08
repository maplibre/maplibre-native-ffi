"""Retry one primary CI failure and its dependent checks once."""

from __future__ import annotations

import json
import os
import subprocess


def retryable(jobs: list[dict]) -> bool:
    if any(job["conclusion"] not in {"success", "failure", "skipped"} for job in jobs):
        return False
    failed = [job["name"] for job in jobs if job["conclusion"] == "failure"]
    required = [
        name
        for name in failed
        if name == "ci-required" or name.startswith("ci-required (")
    ]
    primary = [
        name
        for name in failed
        if name not in required and not name.startswith("coverage / verified (")
    ]
    return len(required) == 1 and len(primary) == 1


def main() -> None:
    run_id = os.environ["CI_RUN_ID"]
    path = f"repos/{os.environ['GITHUB_REPOSITORY']}/actions/runs/{run_id}"
    run = json.loads(subprocess.check_output(["gh", "api", path], text=True))
    if run["run_attempt"] != 1:
        print("This run already has a retry.")
        return
    pages = json.loads(
        subprocess.check_output(
            [
                "gh",
                "api",
                "--paginate",
                "--slurp",
                f"{path}/attempts/1/jobs?per_page=100",
            ],
            text=True,
        )
    )
    if not retryable([job for page in pages for job in page["jobs"]]):
        print("This run does not have exactly one primary failure.")
        return
    subprocess.run(
        ["gh", "api", "--method", "POST", f"{path}/rerun-failed-jobs"], check=True
    )


if __name__ == "__main__":
    main()
