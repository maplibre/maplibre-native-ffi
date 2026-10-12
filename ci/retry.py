"""Retry a CI run once when only infrastructure steps failed."""

from __future__ import annotations

import json
import os
import subprocess
import time

# Steps that provision the runner, fetch dependencies, or boot a device. A
# failure here says nothing about the code under test, so a rerun is worth its
# cost. Every other step, test suites above all, fails the run for good.
INFRASTRUCTURE_STEPS = {
    "Set up job",
    "Complete job",
    "Check out repository",
    "Plan coverage",
    "Set up CI dependencies",
    "Resolve Swift packages",
    "Boot Android emulator",
    "Boot OpenHarmony emulator",
    "Boot iOS simulator",
    "Boot tvOS simulator",
}


def infrastructure_step(name: str) -> bool:
    # A post step belongs to the setup action that registered it, such as a
    # cache save or a Gradle daemon shutdown.
    return name in INFRASTRUCTURE_STEPS or name.startswith("Post ")


def failed_in_infrastructure(job: dict) -> bool:
    failed = [
        step["name"]
        for step in job.get("steps", [])
        if step.get("conclusion") == "failure"
    ]
    # A job with no failed step timed out or lost its runner. A timeout can be
    # a hung test, so it does not count as infrastructure.
    return bool(failed) and all(infrastructure_step(name) for name in failed)


def retryable(jobs: list[dict]) -> bool:
    if any(job["conclusion"] not in {"success", "failure", "skipped"} for job in jobs):
        return False
    failed = [job for job in jobs if job["conclusion"] == "failure"]
    required = [
        job
        for job in failed
        if job["name"] == "ci-required" or job["name"].startswith("ci-required (")
    ]
    primary = [
        job
        for job in failed
        if job not in required and not job["name"].startswith("coverage / verified (")
    ]
    return (
        len(required) == 1
        and bool(primary)
        and all(failed_in_infrastructure(job) for job in primary)
    )


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
        print("This run failed outside the infrastructure steps.")
        return
    subprocess.run(
        ["gh", "api", "--method", "POST", f"{path}/rerun-failed-jobs"], check=True
    )


def main() -> None:
    for attempt in range(4):
        try:
            retry_failed_run(os.environ["CI_RUN_ID"])
            return
        except subprocess.CalledProcessError:
            if attempt == 3:
                raise
            # Recheck eligibility as well as retrying the API: a failed POST
            # response may have arrived after GitHub accepted the rerun.
            delay = 2 ** (attempt + 1)
            print(f"GitHub API request failed; checking again in {delay} seconds.")
            time.sleep(delay)


if __name__ == "__main__":
    main()
