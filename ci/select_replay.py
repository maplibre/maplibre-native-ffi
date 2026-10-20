"""Replay coverage selection over the pushes of merged pull requests.

Compares three strategies for each push: full coverage, selection from the
whole pull request diff, and selection from the diff since each unit last ran.
"""

from __future__ import annotations

import argparse
import concurrent.futures
import hashlib
import json
import subprocess

from ci import select

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

    def units(self, base: str, head: str, title: str) -> tuple[set[str], int]:
        """Return the units selected for a range and the tokens spent."""
        log = select.git("log", "--format=%B", f"{base}..{head}")[:4000]
        message = f"{title}\n\n{log}"
        key = hashlib.sha256(f"{base} {head} {message}".encode()).hexdigest()[:20]
        path = self.cache / f"{key}.json"
        if path.exists():
            result, tokens = json.loads(path.read_text()), 0
        else:
            result = select.select(
                base, head, message, config=self.config, model=self.model
            )
            path.write_text(json.dumps(result))
            tokens = result["input_tokens"]
        chosen = {unit for unit, p in result["units"].items() if p >= self.threshold}
        chosen |= select.forced_units(self.config, select.changed_paths(base, head))
        return chosen, tokens


def cost(units: set[str], minutes: dict[str, int]) -> int:
    if "maven" in units:
        units = set(minutes)
    return minutes["hygiene"] + sum(
        minutes[unit] for unit in units if unit != "hygiene"
    )


def replay(pr: dict, selector: Selector, minutes: dict[str, int]) -> dict:
    ids = [unit["id"] for unit in selector.config["units"]]
    last_run: dict[str, str] = {}
    pushes, tokens = [], 0
    for head in pr["commits"]:
        fork = select.git("merge-base", pr["baseRefOid"], head).strip()
        whole, spent = selector.units(fork, head, pr["title"])
        tokens += spent

        # Each unit judges only what changed since it last ran.
        bases: dict[str, list[str]] = {}
        for unit in ids:
            bases.setdefault(last_run.get(unit, fork), []).append(unit)
        ran = set()
        for base, units in bases.items():
            chosen, spent = (
                (whole, 0) if base == fork else selector.units(base, head, pr["title"])
            )
            tokens += spent
            ran |= chosen & set(units)
        if "maven" in ran:
            ran = set(ids)
        for unit in ran:
            last_run[unit] = head
        pushes.append(
            {"head": head, "whole": sorted(whole), "incremental": sorted(ran)}
        )
    return {
        "number": pr["number"],
        "title": pr["title"],
        "pushes": pushes,
        "full": len(pushes) * cost(set(minutes), minutes),
        "whole": sum(cost(set(p["whole"]), minutes) for p in pushes),
        "incremental": sum(cost(set(p["incremental"]), minutes) for p in pushes),
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
    minutes = {"hygiene": config["hygiene_minutes"]}
    minutes |= {unit["id"]: unit["minutes"] for unit in config["units"]}
    selector = Selector(config, args.model, args.threshold)
    prs = [
        pr for pr in pull_requests(args.since) if not args.pr or pr["number"] in args.pr
    ]

    with concurrent.futures.ThreadPoolExecutor(args.jobs) as pool:
        futures = {pool.submit(replay, pr, selector, minutes): pr for pr in prs}
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
        f"runner-minutes: full {full}, whole-PR selection {whole} ({100 * whole // full}%), "
        f"incremental {incremental} ({100 * incremental // full}%)"
    )


if __name__ == "__main__":
    main()
