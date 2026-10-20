"""Ask Jev which labels a new issue or pull request needs.

Labels go only into groups that have none, so a label that a template, a bot,
or a person set always wins. The workflow runs once, when an item opens, and
only adds labels. Groups, meanings, and thresholds live in `ci/labels.toml`.
Run `uv run python -m ci.label_eval` before changing that file.
"""

from __future__ import annotations

import json
import os
import pathlib
import subprocess
import tomllib

from ci import select

CONFIG = pathlib.Path(__file__).with_name("labels.toml")
BODY_LIMIT = 8_000
# Labels are about a change's subject, so the first chunk of its patch is
# enough. The body and file list fit in the rest of the state budget.
PATCH_BUDGET = 48_000


def load_config(path: pathlib.Path = CONFIG) -> dict:
    return tomllib.loads(path.read_text())


def github(path: str):
    return json.loads(subprocess.check_output(["gh", "api", path], text=True))


def describe(repository: str, number: int) -> dict:
    """Describe an item, and a pull request as coverage selection would."""
    issue = github(f"repos/{repository}/issues/{number}")
    item = {
        "type": "pull request" if "pull_request" in issue else "issue",
        "title": issue["title"],
        "body": (issue["body"] or "")[:BODY_LIMIT],
    }
    if "pull_request" not in issue:
        return item
    pr = github(f"repos/{repository}/pulls/{number}")
    base, head = pr["base"]["sha"], pr["head"]["sha"]
    # Fetching a pull request's commits only reads them; nothing runs its code.
    subprocess.run(
        ["git", "fetch", "--quiet", "--no-tags", "--filter=blob:none", "origin"]
        + [base, f"refs/pull/{number}/head"],
        cwd=select.ROOT,
        check=True,
    )
    fork = select.git("merge-base", base, head).strip()
    described = select.describe_change(fork, head, "")
    del described["change"]["message"]
    chunks = select.chunk(described["patches"], PATCH_BUDGET)
    if len(chunks) > 1:
        chunks[0] += "[... the rest of the diff is omitted]\n"
    return item | described["change"] | {"patch": chunks[0]}


def questions(config: dict) -> dict:
    result = {}
    for group_id, group in config["groups"].items():
        meanings = {
            name: " ".join(means.split()) for name, means in group["labels"].items()
        }
        if group.get("choice"):
            result[group_id] = {
                "type": "choice",
                "instructions": (
                    "Which kind of work is this issue or pull request? Judge the "
                    "work it reports, requests, or makes, not the files it touches."
                ),
                "criteria": meanings,
            }
            continue
        rule = {"rule": " ".join(group["rule"].split())} if "rule" in group else {}
        for name, means in meanings.items():
            result[name] = {
                "type": "noul",
                "instructions": {
                    "question": (
                        f"Should this issue or pull request get the `{name}` "
                        "label? A maintainer searching for that label would expect "
                        "to find it."
                    ),
                    "label": means,
                    **rule,
                },
                "criteria": {
                    "true": "What the label describes is a subject of this work.",
                    "false": (
                        "The work does not concern what the label describes, or "
                        "touches it only in passing."
                    ),
                },
            }
    return result


def judge(
    item: dict,
    config: dict,
    api_key: str,
    *,
    model: str = select.DEFAULT_MODEL,
    on_request=None,
) -> dict:
    """Return each label's probability."""
    question_map = questions(config)
    state = {"repository": " ".join(config["overview"].split()), "item": item}
    response = select.ask(state, question_map, model, api_key)
    if on_request:
        on_request({"state": state, "questions": question_map}, response)
    scores = {}
    for key, answer in response["answers"].items():
        if answer["type"] == "choice":
            scores |= answer["probabilities"]
        else:
            scores[key] = answer["noul"]
    return {
        "model": response["model"],
        "labels": scores,
        "input_tokens": response["usage"]["input_tokens"],
    }


def decide(
    config: dict,
    scores: dict[str, float],
    present: set[str],
    thresholds: dict[str, float] | None = None,
) -> dict[str, dict]:
    """Return whether each label is added, and the score or rule that decided it."""
    decisions = {}
    for group_id, group in config["groups"].items():
        names = list(group["labels"])
        if present & set(names):
            for name in names:
                rule = "present" if name in present else "group labeled"
                decisions[name] = {"add": False, "score": scores[name], "rule": rule}
            continue
        threshold = (thresholds or {}).get(group_id, group["threshold"])
        best = max(names, key=lambda name: scores[name])
        passing = [name for name in names if scores[name] >= threshold]
        if group.get("choice"):
            passing = [best] if best in passing else []
        for name in names:
            decisions[name] = {
                "add": name in passing,
                "score": scores[name],
                "rule": "",
            }
        if group.get("required") and not passing:
            decisions[best] |= {"add": True, "rule": "most likely"}
    return decisions


def main() -> None:
    env = dict(os.environ)
    event = json.loads(pathlib.Path(env["GITHUB_EVENT_PATH"]).read_text())
    repository = env["GITHUB_REPOSITORY"]
    number = (event.get("issue") or event["pull_request"])["number"]
    if not env.get("TYPESAFE_API_KEY"):
        print(f"No Jev API key, so #{number} keeps its labels.")
        return
    config = load_config()
    item = describe(repository, number)
    scores = judge(
        item, config, env["TYPESAFE_API_KEY"], on_request=select.log_request
    )["labels"]
    # Read the labels again in case someone labeled the item while Jev judged it.
    labels = github(f"repos/{repository}/issues/{number}/labels")
    decisions = decide(config, scores, {label["name"] for label in labels})
    added = [name for name, entry in decisions.items() if entry["add"]]
    if added:
        subprocess.run(
            ["gh", "api", "--method", "POST"]
            + [f"repos/{repository}/issues/{number}/labels", "--input", "-"],
            input=json.dumps({"labels": added}),
            text=True,
            check=True,
            stdout=subprocess.DEVNULL,
        )

    heading = f"Added {', '.join(added) or 'no labels'} to #{number}."
    rows = ["| | Label | Score | Rule |", "| --- | --- | --- | --- |"]
    for name, entry in decisions.items():
        mark = "✅" if entry["add"] else "➖"
        rows.append(f"| {mark} | {name} | {entry['score']:.2f} | {entry['rule']} |")
    print("\n".join([heading, *rows]))
    with pathlib.Path(env["GITHUB_STEP_SUMMARY"]).open("a") as summary:
        print(f"{heading}\n\n" + "\n".join(rows), file=summary)


if __name__ == "__main__":
    main()
