"""Evaluate coverage selection against history.

`commits` asks about past commits one at a time and compares the answers with
the labels in `ci/select_cases.toml`. `prs` replays the pushes of merged pull
requests through the CI planner and compares the runner time of the jobs it
selects with full coverage. It treats each commit of a pull request's final
history as a push, so it approximates force pushes and multi-commit pushes. Both cache Jev's answers under `build/ci-select/`.
"""

from __future__ import annotations

import argparse
import concurrent.futures
import json
import os
import pathlib
import subprocess
import tomllib
from datetime import datetime

from ci import plan, select
from ci.coverage import (
    HYGIENE,
    dependencies,
    job_name,
    selected_jobs,
    unit_jobs,
)
from ci.workflow import load_configuration

CASES = pathlib.Path(__file__).with_name("select_cases.toml")
THRESHOLDS = (0.3, 0.4, 0.5, 0.6, 0.7)


def gh(*args: str) -> str:
    return subprocess.check_output(["gh", *args], text=True)


def repository() -> str:
    return gh("repo", "view", "--json", "nameWithOwner", "-q", ".nameWithOwner").strip()


def commits(args, config: dict, cache: pathlib.Path) -> None:
    cases = {
        select.git("rev-parse", case["rev"]).strip(): case
        for case in tomllib.loads(CASES.read_text())["cases"]
    }
    if args.range:
        since = [f"--since={args.since}"] if args.since else []
        revs = select.git("rev-list", "--no-merges", *since, args.range).split()
    else:
        revs = [select.git("rev-parse", rev).strip() for rev in args.revs] or list(
            cases
        )

    def judge(sha: str) -> dict:
        message = select.git("log", "-1", "--format=%B", sha)
        result = select.select(
            f"{sha}^", sha, message, config=config, model=args.model, cache=cache
        )
        forced = select.forced_units(config, select.changed_paths(f"{sha}^", sha))
        subject = message.splitlines()[0]
        return {"sha": sha, "subject": subject, "forced": forced, **result}

    with concurrent.futures.ThreadPoolExecutor(args.jobs) as pool:
        results = list(pool.map(judge, revs))

    def chosen(result: dict, threshold: float) -> set[str]:
        units = {unit for unit, p in result["units"].items() if p >= threshold}
        return units | set(result["forced"])

    units = [unit["id"] for unit in config["units"]]
    print("| commit | subject | " + " | ".join(units) + " |")
    print("| --- | --- | " + " | ".join("---" for _ in units) + " |")
    for result in results:
        case = cases.get(result["sha"])
        cells = []
        for unit in units:
            run = unit in chosen(result, args.threshold)
            mark = ""
            if case and run and unit not in case["must"] + case["ok"]:
                mark = "**EXTRA** "
            elif case and not run and unit in case["must"]:
                mark = "**MISS** "
            forced = "F" if unit in result["forced"] else ""
            cells.append(f"{mark}{result['units'][unit]:.2f}{forced}")
        print(
            f"| {result['sha'][:9]} | {result['subject'][:50]} | {' | '.join(cells)} |"
        )

    labeled = [result for result in results if result["sha"] in cases]
    if labeled:
        print("\n| threshold | misses | extras |\n| --- | --- | --- |")
        for threshold in THRESHOLDS:
            misses = extras = 0
            for result in labeled:
                case, run = cases[result["sha"]], chosen(result, threshold)
                misses += len(set(case["must"]) - run)
                extras += len(run - set(case["must"]) - set(case["ok"]))
            print(f"| {threshold} | {misses} | {extras} |")
    print(
        f"\n{len(results)} commits, {sum(r['input_tokens'] for r in results)} new tokens"
    )


def job_minutes(repo: str, owned: dict[str, list[str]]) -> dict[str, float]:
    """Read each job's runner time from the latest full main run."""
    run = json.loads(
        gh(
            "run",
            "list",
            "-R",
            repo,
            "--workflow",
            plan.WORKFLOW,
            "--branch",
            "main",
            "--event",
            "push",
            "--status",
            "success",
            "--limit",
            "1",
            "--json",
            "databaseId",
        )
    )[0]["databaseId"]
    jobs = json.loads(gh("run", "view", str(run), "-R", repo, "--json", "jobs"))["jobs"]
    # Runs from before the single workflow nested their jobs under `coverage`.
    for entry in jobs:
        entry["name"] = entry["name"].removeprefix("coverage / ")
    minutes = {}
    for job in (job for owned_jobs in owned.values() for job in owned_jobs):
        name = job_name(job)
        minutes[job] = sum(
            (
                datetime.fromisoformat(entry["completedAt"])
                - datetime.fromisoformat(entry["startedAt"])
            ).total_seconds()
            / 60
            for entry in jobs
            if entry["name"] == name or entry["name"].startswith(f"{name} / ")
        )
    return minutes


def merged_pull_requests(
    repo: str, since: str, numbers: list[int] | None
) -> list[dict]:
    owner, name = repo.split("/")
    query = (
        "query($owner: String!, $name: String!, $endCursor: String) {"
        " repository(owner: $owner, name: $name) {"
        " pullRequests(states: MERGED, first: 50, after: $endCursor,"
        " orderBy: {field: CREATED_AT, direction: DESC}) {"
        " pageInfo { hasNextPage endCursor }"
        " nodes { number title mergedAt baseRefOid headRefName"
        " commits(first: 100) { totalCount nodes { commit { oid } } } } } } }"
    )
    jq = (
        ".data.repository.pullRequests.nodes[]"
        f' | select(.mergedAt >= "{since}" and .commits.totalCount > 1)'
        " | {number, title, baseRefOid, headRefName,"
        " commits: [.commits.nodes[].commit.oid]}"
    )
    output = gh(
        "api",
        "graphql",
        "--paginate",
        "-f",
        f"query={query}",
        "-F",
        f"owner={owner}",
        "-F",
        f"name={name}",
        "--jq",
        jq,
    )
    prs = [json.loads(line) for line in output.splitlines()]
    prs = [pr for pr in prs if not numbers or pr["number"] in numbers]
    refspecs = [
        f"+refs/pull/{pr['number']}/head:refs/remotes/origin/pr/{pr['number']}"
        for pr in prs
    ]
    subprocess.run(
        ["git", "fetch", "--quiet", "origin", *refspecs], cwd=select.ROOT, check=True
    )
    return prs


def replay(pr: dict, config: dict, owned: dict, needs: dict, choose) -> list[dict]:
    """Plan each push as CI would, assuming every selected job passes."""
    runs: list[tuple[dict, list[dict]]] = []

    def api(path: str) -> dict:
        if "/workflows/" in path:
            return {"workflow_runs": [run for run, _ in runs]}
        run_id = int(path.split("/runs/")[1].split("/")[0])
        jobs = next(jobs for run, jobs in runs if run["id"] == run_id)
        return {"total_count": len(jobs), "jobs": jobs}

    pushes = []
    for index, head in enumerate(pr["commits"]):
        event = {
            "pull_request": {
                "number": pr["number"],
                "title": pr["title"],
                "labels": [],
                "head": {"sha": head, "ref": pr["headRefName"]},
                "base": {"sha": pr["baseRefOid"]},
            }
        }
        env = {"GITHUB_REPOSITORY": "replay", "GITHUB_RUN_ID": str(index)}
        decisions = plan.plan(
            config, owned, "pull_request", event, env, api=api, choose=choose
        )
        units = [unit for unit, entry in decisions.items() if entry["run"]]
        jobs = selected_jobs(owned, needs, units)
        # The Kotlin Maven reusable workflow reports nested job names.
        reported = [
            {
                "name": job_name(job) + (" / verify" if job == "kotlin-maven" else ""),
                "conclusion": "success",
            }
            for job in jobs
        ]
        runs.append(
            (
                {
                    "id": index,
                    "head_sha": head,
                    "pull_requests": [{"number": pr["number"]}],
                },
                reported,
            )
        )
        pushes.append({"head": head, "decisions": decisions, "jobs": jobs})
    return pushes


def prs(args, config: dict, cache: pathlib.Path) -> None:
    repo = repository()
    source, presets = load_configuration(select.ROOT)
    owned = unit_jobs(config, source, presets)
    needs = dependencies(source, presets)
    minutes = job_minutes(repo, owned)
    full = sum(minutes.values())
    choose = plan.jev_choice(
        config, os.environ["TYPESAFE_API_KEY"], model=args.model, cache=cache
    )
    pulls = merged_pull_requests(repo, args.since, args.pr)

    with concurrent.futures.ThreadPoolExecutor(args.jobs) as pool:
        replays = dict(
            zip(
                (pr["number"] for pr in pulls),
                pool.map(lambda pr: replay(pr, config, owned, needs, choose), pulls),
            )
        )
    (cache / "replay.json").write_text(json.dumps(replays, indent=2))

    pushes = [push for pushes in replays.values() for push in pushes]
    if not pushes:
        print("No pull request pushes to replay.")
        return
    spent = sum(minutes[job] for push in pushes for job in push["jobs"])
    fallbacks = sum(
        push["decisions"][HYGIENE]["rule"].startswith("full: selection failed")
        for push in pushes
    )
    print(
        f"{len(replays)} pull requests, {len(pushes)} pushes, {fallbacks} fallbacks to full"
    )
    print(
        f"runner minutes: full {full * len(pushes):.0f}, "
        f"selected {spent:.0f} ({100 * spent / (full * len(pushes)):.0f}%)"
    )


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--model", default=select.DEFAULT_MODEL)
    parser.add_argument("--jobs", type=int, default=8)
    commands = parser.add_subparsers(dest="command", required=True)
    on_commits = commands.add_parser("commits", help="judge commits against labels")
    on_commits.add_argument("revs", nargs="*", help="defaults to the labeled commits")
    on_commits.add_argument("--range", help="every non-merge commit in a git range")
    on_commits.add_argument("--since", help="limit --range to commits after a date")
    on_commits.add_argument("--threshold", type=float, default=select.THRESHOLD)
    on_prs = commands.add_parser("prs", help="replay merged pull requests")
    on_prs.add_argument("--since", required=True, help="merged on or after a date")
    on_prs.add_argument("--pr", type=int, action="append", help="only these numbers")
    args = parser.parse_args()

    config = select.load_config()
    cache = select.ROOT / "build" / "ci-select" / args.model
    {"commits": commits, "prs": prs}[args.command](args, config, cache)


if __name__ == "__main__":
    main()
