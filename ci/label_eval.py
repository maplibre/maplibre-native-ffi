"""Evaluate labeling against hand-labeled issues and pull requests.

Judges each item as if it had just opened with no labels, and compares the
labels Jev would add with `ci/label_cases.toml` at a range of thresholds for
each group. Caches Jev's answers under `build/ci-label/`.
"""

from __future__ import annotations

import argparse
import concurrent.futures
import hashlib
import json
import os
import tomllib

from ci import label, select
from ci.select_eval import repository

CASES = label.CONFIG.with_name("label_cases.toml")
THRESHOLDS = (0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("numbers", nargs="*", type=int, help="defaults to the cases")
    parser.add_argument("--model", default=select.DEFAULT_MODEL)
    parser.add_argument("--jobs", type=int, default=8)
    args = parser.parse_args()

    config = label.load_config()
    cache = select.ROOT / "build" / "ci-label" / args.model
    cache.mkdir(parents=True, exist_ok=True)
    cases = {
        int(number): {key: set(value.split()) for key, value in case.items()}
        for number, case in tomllib.loads(CASES.read_text())["cases"].items()
    }
    repo = repository()

    def judge(number: int) -> dict:
        item = label.describe(repo, number)
        material = json.dumps([args.model, item, label.questions(config)])
        path = cache / f"{hashlib.sha256(material.encode()).hexdigest()[:24]}.json"
        if path.exists():
            result = json.loads(path.read_text()) | {"input_tokens": 0}
        else:
            result = label.judge(
                item, config, os.environ["TYPESAFE_API_KEY"], model=args.model
            )
            path.write_text(json.dumps(result))
        return {"number": number, "title": item["title"], **result}

    with concurrent.futures.ThreadPoolExecutor(args.jobs) as pool:
        results = list(pool.map(judge, args.numbers or list(cases)))

    def added(result: dict, thresholds: dict | None = None) -> set[str]:
        decisions = label.decide(config, result["labels"], set(), thresholds)
        return {name for name, entry in decisions.items() if entry["add"]}

    def cell(result: dict, names: set[str]) -> str:
        return " ".join(f"{n} {result['labels'][n]:.2f}" for n in sorted(names))

    print("| item | title | added | missed | extra |\n| --- | --- | --- | --- | --- |")
    for result in results:
        chosen = added(result)
        case = cases.get(result["number"], {"must": chosen, "ok": set()})
        print(
            f"| #{result['number']} | {result['title'][:50]} "
            f"| {cell(result, chosen)} | {cell(result, case['must'] - chosen)} "
            f"| {cell(result, chosen - case['must'] - case['ok'])} |"
        )

    labeled = [result for result in results if result["number"] in cases]
    if labeled:
        print("\n| group | threshold | misses | extras |\n| --- | --- | --- | --- |")
        for group_id, group in config["groups"].items():
            names = set(group["labels"])
            for threshold in THRESHOLDS:
                misses = extras = 0
                for result in labeled:
                    case = cases[result["number"]]
                    chosen = added(result, {group_id: threshold}) & names
                    misses += len(case["must"] & names - chosen)
                    extras += len(chosen - case["must"] - case["ok"])
                mark = " (current)" if threshold == group["threshold"] else ""
                print(f"| {group_id} | {threshold}{mark} | {misses} | {extras} |")
    print(
        f"\n{len(results)} items, {sum(r['input_tokens'] for r in results)} new tokens"
    )


if __name__ == "__main__":
    main()
