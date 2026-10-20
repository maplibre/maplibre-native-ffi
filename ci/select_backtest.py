"""Replay coverage selection over past commits and compare it with labels."""

from __future__ import annotations

import argparse
import concurrent.futures
import json
import pathlib
import tomllib

from ci import select

CASES = pathlib.Path(__file__).with_name("select_cases.toml")
THRESHOLDS = (0.2, 0.3, 0.4, 0.5, 0.6, 0.7)


def commit(rev: str) -> dict:
    sha, subject, message = select.git(
        "log", "-1", "--format=%H%x00%s%x00%B", rev
    ).split("\0", 2)
    return {"sha": sha, "subject": subject, "message": message}


def run(rev: str, cache: pathlib.Path, model: str, config: dict, force: bool) -> dict:
    info = commit(rev)
    path = cache / f"{info['sha']}.json"
    if path.exists() and not force:
        return json.loads(path.read_text())
    result = select.select(
        f"{info['sha']}^", info["sha"], info["message"], config=config, model=model
    )
    result.update(info)
    path.write_text(json.dumps(result, indent=2))
    return result


def selected(result: dict, threshold: float) -> set[str]:
    jev = {unit for unit, p in result["units"].items() if p >= threshold}
    return jev | set(result["forced"])


def score(
    result: dict, case: dict | None, threshold: float
) -> tuple[list[str], list[str]]:
    """Return (missed must units, selected units outside must and ok)."""
    if case is None:
        return [], []
    selected_units = selected(result, threshold)
    allowed = set(case["must"]) | set(case["ok"])
    return sorted(set(case["must"]) - selected_units), sorted(selected_units - allowed)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "revs", nargs="*", help="commits to replay; defaults to the labeled cases"
    )
    parser.add_argument("--range", help="replay every non-merge commit in a git range")
    parser.add_argument("--since", help="limit --range to commits after this date")
    parser.add_argument("--model", default=select.DEFAULT_MODEL)
    parser.add_argument("--threshold", type=float, default=select.THRESHOLD)
    parser.add_argument("--jobs", type=int, default=6)
    parser.add_argument("--force", action="store_true", help="ignore cached answers")
    args = parser.parse_args()

    config = select.load_config()
    cases = {
        select.git("rev-parse", case["rev"]).strip(): case
        for case in tomllib.loads(CASES.read_text())["cases"]
    }
    if args.range:
        since = [f"--since={args.since}"] if args.since else []
        revs = select.git("rev-list", "--no-merges", *since, args.range).split()
    else:
        revs = args.revs or list(cases)

    digest = select.fingerprint(config)
    cache = select.ROOT / "build" / "ci-select" / args.model / digest
    cache.mkdir(parents=True, exist_ok=True)

    with concurrent.futures.ThreadPoolExecutor(args.jobs) as pool:
        futures = {
            pool.submit(run, rev, cache, args.model, config, args.force): rev
            for rev in revs
        }
        results = []
        for future in concurrent.futures.as_completed(futures):
            try:
                results.append(future.result())
            except Exception as error:  # noqa: BLE001 - report and keep replaying the rest.
                print(f"{futures[future]}: {error}")
    order = {
        select.git("rev-parse", rev).strip(): index for index, rev in enumerate(revs)
    }
    results.sort(key=lambda result: order.get(result["sha"], 0))
    for result in results:
        paths = select.changed_paths(f"{result['sha']}^", result["sha"])
        result["forced"] = sorted(select.forced_units(config, paths))
        result["selected"] = sorted(selected(result, args.threshold))
    (cache / "results.json").write_text(json.dumps(results))

    units = [unit["id"] for unit in config["units"]]
    lines = [
        (
            f"model {results[0]['model'] if results else args.model}, "
            f"config {digest}, threshold {args.threshold}"
        ),
        "",
        "| commit | subject | chunks | " + " | ".join(units) + " |",
        "| --- | --- | --- | " + " | ".join("---" for _ in units) + " |",
    ]
    for result in results:
        case = cases.get(result["sha"])
        missed, extra = score(result, case, args.threshold)
        cells = []
        for unit in units:
            p = result["units"][unit]
            mark = (
                "**MISS** " if unit in missed else "**EXTRA** " if unit in extra else ""
            )
            forced = "F" if unit in result["forced"] else ""
            cells.append(f"{mark}{p:.2f}{forced}")
        lines.append(
            f"| {result['sha'][:9]} | {result['subject'][:60]} | {len(result['chunks'])} | "
            + " | ".join(cells)
            + " |"
        )

    labeled = [result for result in results if result["sha"] in cases]
    if labeled:
        lines += ["", "| threshold | misses | extras |", "| --- | --- | --- |"]
        for threshold in THRESHOLDS:
            totals = [
                score(result, cases[result["sha"]], threshold) for result in labeled
            ]
            lines.append(
                f"| {threshold} | {sum(len(m) for m, _ in totals)} | "
                f"{sum(len(e) for _, e in totals)} |"
            )
    tokens = sum(result["input_tokens"] for result in results)
    lines += ["", f"{len(results)} commits, {tokens} input tokens"]
    report = "\n".join(lines)
    (cache / "report.md").write_text(report + "\n")
    print(report)


if __name__ == "__main__":
    main()
