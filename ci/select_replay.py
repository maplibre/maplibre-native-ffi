"""Replay coverage selection over the pushes of merged pull requests.

Compares three strategies for each push: full coverage, selection from the
whole pull request diff, and selection from each unit's own changes since it
last ran, as CI selects.
"""

from __future__ import annotations

import argparse
import concurrent.futures
import hashlib
import json
import subprocess

from ci import select
from ci.coverage import HYGIENE, dependencies, selected_jobs, unit_jobs
from ci.workflow import load_configuration

QUERY = """
query($endCursor: String) {
  repository(owner: "maplibre", name: "maplibre-native-ffi") {
    pullRequests(states: MERGED, first: 50, after: $endCursor,
                 orderBy: {field: CREATED_AT, direction: DESC}) {
      pageInfo { hasNextPage endCursor }
      nodes {
        number title mergedAt baseRefOid
        commits(first: 100) { totalCount nodes { commit { oid } } }
      }
    }
  }
}
"""


def pull_requests(since: str) -> list[dict]:
    output = subprocess.check_output(
        [
            "gh",
            "api",
            "graphql",
            "--paginate",
            "-f",
            f"query={QUERY}",
            "--jq",
            (
                ".data.repository.pullRequests.nodes[]"
                f' | select(.mergedAt >= "{since}" and .commits.totalCount > 1)'
                " | {number, title, baseRefOid,"
                " commits: [.commits.nodes[].commit.oid]}"
            ),
        ],
        text=True,
    )
    return [json.loads(line) for line in output.splitlines()]


class Selector:
    """Cache Jev answers by the exact change sent."""

    def __init__(self, config: dict, model: str, threshold: float):
        self.config, self.model, self.threshold = config, model, threshold
        self.cache = (
            select.ROOT
            / "build"
            / "ci-select"
            / model
            / select.fingerprint(config)
            / "ranges"
        )
        self.cache.mkdir(parents=True, exist_ok=True)

    def units(
        self, judged: str, head: str, title: str, since: str
    ) -> tuple[set[str], int]:
        """Return the units selected for a change and the tokens spent."""
        if not select.changed_paths(judged, head):
            return set(), 0
        log = select.git("log", "--format=%B", f"{since}..{head}")[:4000]
        message = f"{title}\n\n{log}"
        key = hashlib.sha256(f"{judged} {head} {message}".encode()).hexdigest()[:20]
        path = self.cache / f"{key}.json"
        if path.exists():
            result, tokens = json.loads(path.read_text()), 0
        else:
            result = select.select(
                judged, head, message, config=self.config, model=self.model
            )
            path.write_text(json.dumps(result))
            tokens = result["input_tokens"]
        chosen = {unit for unit, p in result["units"].items() if p >= self.threshold}
        chosen |= select.forced_units(self.config, select.changed_paths(judged, head))
        return chosen, tokens


class Coverage:
    """Expand units into the jobs CI runs, and price them."""

    def __init__(self, config: dict):
        source, presets = load_configuration(select.ROOT)
        self.owned = unit_jobs(config, source, presets)
        self.needs = dependencies(source, presets)
        self.minutes = {HYGIENE: config["hygiene_minutes"]}
        self.minutes |= {unit["id"]: unit["minutes"] for unit in config["units"]}

    def run(self, units: set[str]) -> tuple[set[str], float]:
        """Return the units whose jobs all run, and the runner minutes spent."""
        jobs = set(selected_jobs(self.owned, self.needs, {HYGIENE, *units}))
        complete = {unit for unit, owned in self.owned.items() if set(owned) <= jobs}
        # A packaging job can pull in part of another unit.
        minutes = sum(
            self.minutes[unit] * len(set(owned) & jobs) / len(owned)
            for unit, owned in self.owned.items()
        )
        return complete - {HYGIENE}, minutes


def replay(pr: dict, selector: Selector, coverage: Coverage) -> dict:
    ids = [unit["id"] for unit in selector.config["units"]]
    last_run: dict[str, str] = {}
    pushes, tokens = [], 0
    for head in pr["commits"]:
        fork = select.git("merge-base", pr["baseRefOid"], head).strip()
        whole, spent = selector.units(fork, head, pr["title"], fork)
        tokens += spent

        # Each unit judges only its pull request's own changes since it last ran.
        bases: dict[str, list[str]] = {}
        for unit in ids:
            bases.setdefault(last_run.get(unit, fork), []).append(unit)
        chosen_units = set()
        for base, units in bases.items():
            if base == fork:
                chosen = whole
            else:
                judged = select.own_changes_base(base, fork)
                chosen, spent = selector.units(judged, head, pr["title"], base)
                tokens += spent
            chosen_units |= chosen & set(units)
        ran, minutes = coverage.run(chosen_units)
        for unit in ran:
            last_run[unit] = head
        pushes.append(
            {
                "head": head,
                "whole": sorted(whole),
                "incremental": sorted(ran),
                "whole_minutes": coverage.run(whole)[1],
                "incremental_minutes": minutes,
            }
        )
    return {
        "number": pr["number"],
        "title": pr["title"],
        "pushes": pushes,
        "full": len(pushes) * coverage.run(set(ids))[1],
        "whole": sum(push["whole_minutes"] for push in pushes),
        "incremental": sum(push["incremental_minutes"] for push in pushes),
        "tokens": tokens,
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--since", required=True, help="merged on or after this date")
    parser.add_argument("--model", default=select.DEFAULT_MODEL)
    parser.add_argument("--threshold", type=float, default=select.THRESHOLD)
    parser.add_argument("--jobs", type=int, default=8)
    parser.add_argument(
        "--pr", type=int, action="append", help="replay only these pull requests"
    )
    args = parser.parse_args()

    config = select.load_config()
    coverage = Coverage(config)
    selector = Selector(config, args.model, args.threshold)
    prs = [
        pr for pr in pull_requests(args.since) if not args.pr or pr["number"] in args.pr
    ]

    with concurrent.futures.ThreadPoolExecutor(args.jobs) as pool:
        futures = {pool.submit(replay, pr, selector, coverage): pr for pr in prs}
        results = []
        for future in concurrent.futures.as_completed(futures):
            try:
                results.append(future.result())
            except Exception as error:  # noqa: BLE001 - report and keep replaying the rest.
                print(f"#{futures[future]['number']}: {error}")
    results.sort(key=lambda result: -result["number"])
    (selector.cache.parent / "replay.json").write_text(json.dumps(results, indent=2))

    full, whole, incremental = (
        sum(r[k] for r in results) for k in ("full", "whole", "incremental")
    )
    pushes = sum(len(r["pushes"]) for r in results)
    print(
        f"{len(results)} pull requests, {pushes} pushes, {sum(r['tokens'] for r in results)} new tokens"
    )
    print(
        f"runner-minutes: full {full:.0f}, "
        f"whole-PR selection {whole:.0f} ({100 * whole / full:.0f}%), "
        f"incremental {incremental:.0f} ({100 * incremental / full:.0f}%)"
    )


if __name__ == "__main__":
    main()
