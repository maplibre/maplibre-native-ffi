"""Gate snapshot publishing on the inputs each component actually consumes.

`hash` digests one component's inputs at a commit. `plan` digests every
component at the target commit and at the commit it last published from, and
reports the ones that differ. `record` moves each published component's state
tag, and `read` prints where those tags point.

The state is one floating tag per component under `snapshot-state/`, so the
commit a component last published from is a first-class ref: `git ls-remote
--tags` answers it without an API call, and the annotated tag message carries
the run that published it. Digests are recomputed from the tagged commit rather
than stored, so editing a component's scope changes the comparison instead of
invalidating it.
"""

import argparse
import datetime
import json
import os
import pathlib
import sys
import urllib.error
import urllib.request

from ci.snapshots import (
    Component,
    UncoveredPaths,
    component_hash,
    entries,
    load,
    state_tag,
)

ROOT = pathlib.Path.cwd()


def _token() -> str:
    for name in ("GH_TOKEN", "GITHUB_TOKEN"):
        token = os.environ.get(name)
        if token:
            return token
    raise SystemExit("error: GH_TOKEN or GITHUB_TOKEN is required to reach GitHub")


def _api(method: str, path: str, payload: dict | None = None) -> object | None:
    """Call the repository's GitHub API, reporting a 404 as None."""
    repository = os.environ.get("GITHUB_REPOSITORY")
    if not repository:
        raise SystemExit("error: GITHUB_REPOSITORY is required to reach GitHub")
    base = os.environ.get("GITHUB_API_URL", "https://api.github.com")
    request = urllib.request.Request(
        f"{base}/repos/{repository}{path}",
        method=method,
        data=None if payload is None else json.dumps(payload).encode(),
        headers={
            "Accept": "application/vnd.github+json",
            "Authorization": f"Bearer {_token()}",
            "Content-Type": "application/json",
            "X-GitHub-Api-Version": "2022-11-28",
        },
    )
    try:
        with urllib.request.urlopen(request) as response:
            return json.loads(response.read() or b"null")
    except urllib.error.HTTPError as error:
        if error.code == 404:
            return None
        detail = error.read().decode(errors="replace")
        raise SystemExit(
            f"error: {method} {path} failed with HTTP {error.code}: {detail}"
        ) from error


def published_commit(name: str) -> str | None:
    """The commit a component last published from, or None before its first."""
    reference = _api("GET", f"/git/ref/tags/{state_tag(name)}")
    if reference is None:
        return None
    target = reference["object"]
    # An annotated tag names a tag object, which in turn names the commit.
    if target["type"] == "tag":
        return _api("GET", f"/git/tags/{target['sha']}")["object"]["sha"]
    return target["sha"]


def move_tag(name: str, commit: str, message: str) -> None:
    """Point a component's state tag at the commit it just published from."""
    tag = state_tag(name)
    annotated = _api(
        "POST",
        "/git/tags",
        {"tag": tag, "message": message, "object": commit, "type": "commit"},
    )
    if _api("GET", f"/git/ref/tags/{tag}") is None:
        _api("POST", "/git/refs", {"ref": f"refs/tags/{tag}", "sha": annotated["sha"]})
        return
    _api("PATCH", f"/git/refs/tags/{tag}", {"sha": annotated["sha"], "force": True})


def _run_url() -> str:
    server = os.environ.get("GITHUB_SERVER_URL", "https://github.com")
    repository = os.environ.get("GITHUB_REPOSITORY", "")
    return f"{server}/{repository}/actions/runs/{os.environ.get('GITHUB_RUN_ID', '')}"


def _boolean(value: str) -> bool:
    lowered = value.strip().lower()
    if lowered in {"true", "1", "yes"}:
        return True
    if lowered in {"", "false", "0", "no"}:
        return False
    raise argparse.ArgumentTypeError(f"expected a boolean, got {value!r}")


def _component(components: dict[str, Component], name: str) -> Component:
    if name not in components:
        known = ", ".join(sorted(components))
        raise SystemExit(f"error: unknown component {name!r}; known: {known}")
    return components[name]


def _selection(components: dict[str, Component], requested: str) -> set[str]:
    if requested.strip() in {"", "all"}:
        return set(components)
    names = {name.strip() for name in requested.split(",") if name.strip()}
    for name in sorted(names):
        _component(components, name)
    return names


def _emit_outputs(report: dict[str, dict]) -> None:
    output = os.environ.get("GITHUB_OUTPUT")
    if not output:
        return
    with open(output, "a", encoding="utf-8") as file:
        file.writelines(
            f"publish_{name}={str(entry['publish']).lower()}\n"
            for name, entry in sorted(report.items())
        )


def command_hash(arguments: argparse.Namespace) -> int:
    components = load(ROOT)
    component = _component(components, arguments.component)
    print(component_hash(component, entries(ROOT, arguments.ref)))
    return 0


def published_hash(component: Component, commit: str | None) -> str | None:
    """The component's digest at the commit it last published from.

    None when it has never published, and None again when today's rules leave
    that commit unclassified. Deleting the last path a rule named forces that
    rule out for `ci:check-snapshot-scopes` to pass, which leaves the earlier
    tree unreadable; republishing is both the safe answer and the right one,
    since a deleted input is a change worth publishing.
    """
    if commit is None:
        return None
    try:
        return component_hash(component, entries(ROOT, commit))
    except UncoveredPaths as uncovered:
        sample = ", ".join(sorted(uncovered.paths)[:3])
        print(
            f"warning: {component.name}: {commit[:9]} holds paths this scope no "
            f"longer classifies ({sample}), so it republishes",
            file=sys.stderr,
        )
        return None


def command_plan(arguments: argparse.Namespace) -> int:
    components = load(ROOT)
    selected = _selection(components, arguments.components)
    found = entries(ROOT, arguments.ref)

    report = {}
    for name, component in components.items():
        digest = component_hash(component, found)
        previous = published_commit(name)
        published = published_hash(component, previous)
        changed = published != digest
        report[name] = {
            "hash": digest,
            "publishedCommit": previous,
            "publishedHash": published,
            "changed": changed,
            "publish": name in selected and (changed or arguments.force),
        }

    _emit_outputs(report)
    print(json.dumps(report, indent=2, sort_keys=True))
    return 0


def command_read(_: argparse.Namespace) -> int:
    print(
        json.dumps(
            {name: published_commit(name) for name in load(ROOT)},
            indent=2,
            sort_keys=True,
        )
    )
    return 0


def command_record(arguments: argparse.Namespace) -> int:
    components = load(ROOT)
    for name in arguments.components:
        _component(components, name)

    if not arguments.components:
        print("no components published; leaving the state tags alone")
        return 0

    commit = os.environ.get("SNAPSHOT_SHA")
    if not commit:
        raise SystemExit("error: SNAPSHOT_SHA is required to record a publish")

    published_at = (
        datetime.datetime.now(datetime.UTC)
        .isoformat(timespec="seconds")
        .replace("+00:00", "Z")
    )
    for name in arguments.components:
        move_tag(
            name,
            commit,
            f"Published the {name} snapshot at {published_at}\n\n{_run_url()}\n",
        )
        print(f"{state_tag(name)} -> {commit}")
    return 0


def parser() -> argparse.ArgumentParser:
    root = argparse.ArgumentParser(description=__doc__)
    commands = root.add_subparsers(dest="command", required=True)

    hash_command = commands.add_parser("hash", help="digest one component's inputs")
    hash_command.add_argument("component")
    hash_command.add_argument("ref", nargs="?", default="HEAD")
    hash_command.set_defaults(handler=command_hash)

    plan_command = commands.add_parser("plan", help="report what to publish")
    plan_command.add_argument("ref", nargs="?", default="HEAD")
    plan_command.add_argument(
        "--components",
        default="all",
        help="comma-separated component names, or 'all'",
    )
    plan_command.add_argument(
        "--force",
        type=_boolean,
        default=False,
        help="publish selected components even when their inputs are unchanged",
    )
    plan_command.set_defaults(handler=command_plan)

    read_command = commands.add_parser("read", help="print where the state tags point")
    read_command.set_defaults(handler=command_read)

    record_command = commands.add_parser("record", help="move published state tags")
    record_command.add_argument("components", nargs="*", metavar="component")
    record_command.set_defaults(handler=command_record)

    return root


def main() -> int:
    arguments = parser().parse_args()
    try:
        return arguments.handler(arguments)
    except UncoveredPaths as uncovered:
        print(f"error: {uncovered}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
