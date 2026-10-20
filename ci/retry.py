"""Retry one primary CI failure and the required check once."""

from __future__ import annotations

import json
import os
import subprocess


def retryable(jobs: list[dict]) -> bool:
    if any(job["conclusion"] not in {"success", "failure", "skipped"} for job in jobs):
        return False
    failed = [job["name"] for job in jobs if job["conclusion"] == "failure"]
    primary = [name for name in failed if name != "ci-required"]
    return "ci-required" in failed and len(primary) == 1


def retry_failed_run(run_id: str) -> None:
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


def main() -> None:
    # A retry is an optimization; an API failure leaves the run for a person.
    retry_failed_run(os.environ["CI_RUN_ID"])


if __name__ == "__main__":
    main()
