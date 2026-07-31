"""Component input scopes for change-gated snapshot publishing.

Every publishable component declares an ordered, gitignore-style rule list
naming the tracked paths that feed the artifact it publishes. A `!` prefix
marks a path as not an input, and the last matching rule wins, so a component
narrows or widens what it inherits from `shared` and from the components it
depends on.

No rule is a catch-all: every tracked path must match some rule of every
component, which is what `ci:check-snapshot-scopes` enforces. A new top-level
entry therefore fails that check until someone classifies it, rather than
having its publishing impact decided by default.
"""

from __future__ import annotations

import functools
import hashlib
import pathlib
import re
import subprocess
import tomllib
from typing import NamedTuple


CONFIG = pathlib.Path("ci/snapshots.toml")

# Each component's last publish is recorded as a floating tag pointing at the
# commit it published from. Their own namespace keeps them clear of the
# `unstable-native-snapshot` release tag, which has to move before its assets
# upload and so cannot mark a publish that succeeded.
STATE_TAGS = "snapshot-state"

# Uncovered paths are listed rather than counted, up to a length that still
# reads as an error message.
SAMPLE = 20


class Rule(NamedTuple):
    """One ordered rule, tagged with the table that declared it."""

    pattern: str
    inputs: bool
    origin: str

    @property
    def text(self) -> str:
        return self.pattern if self.inputs else f"!{self.pattern}"


class Component(NamedTuple):
    """A publishable component and the ordered rules covering its inputs."""

    name: str
    description: str
    depends: tuple[str, ...]
    rules: tuple[Rule, ...]


class Entry(NamedTuple):
    """One tracked object at a commit, as `git ls-tree -r` reports it."""

    mode: str
    oid: str
    path: str


class UncoveredPaths(Exception):
    """Raised when a component's rules leave a path at a commit unclassified."""

    def __init__(self, component: Component, paths: list[str]) -> None:
        super().__init__(describe(component, paths))
        self.component = component
        self.paths = paths


def _segment(text: str) -> str:
    """Translate one path segment, where `*` and `?` stop at a separator."""
    return "".join(
        "[^/]*" if char == "*" else "[^/]" if char == "?" else re.escape(char)
        for char in text
    )


@functools.cache
def _matcher(pattern: str) -> re.Pattern[str]:
    """Compile a gitignore-style pattern into a whole-path expression."""
    # A separator anywhere but the end anchors the pattern at the repository
    # root; anything else matches at any depth, as gitignore reads it.
    anchored = "/" in pattern.rstrip("/")
    segments = pattern.strip("/").split("/")
    parts = []
    for index, segment in enumerate(segments):
        last = index == len(segments) - 1
        if segment == "**":
            parts.append(".*" if last else "(?:[^/]+/)*")
        else:
            parts.append(_segment(segment) + ("" if last else "/"))
    return re.compile(("" if anchored else "(?:[^/]+/)*") + "".join(parts))


def matches(pattern: str, path: str) -> bool:
    """Whether the pattern names the path, or a directory holding it."""
    matcher = _matcher(pattern)
    if matcher.fullmatch(path):
        return True
    return any(
        matcher.fullmatch(path[:index])
        for index, char in enumerate(path)
        if char == "/"
    )


def classify(rules: tuple[Rule, ...], path: str) -> bool | None:
    """Whether the path is an input; None when no rule mentions it."""
    # Last match wins, so the first match scanning backwards is the verdict.
    for rule in reversed(rules):
        if matches(rule.pattern, path):
            return rule.inputs
    return None


def _rules(patterns: object, origin: str) -> list[Rule]:
    if not isinstance(patterns, list):
        raise SystemExit(
            f"error: {CONFIG.as_posix()}: {origin} must be a list of patterns"
        )
    return [
        Rule(str(pattern).removeprefix("!"), not str(pattern).startswith("!"), origin)
        for pattern in patterns
    ]


def _order(name: str, declared: dict[str, dict], trail: tuple[str, ...]) -> list[str]:
    """Every component `name` inherits from, dependencies first, once each."""
    if name in trail:
        cycle = " -> ".join((*trail, name))
        raise SystemExit(f"error: {CONFIG.as_posix()}: dependency cycle {cycle}")
    if name not in declared:
        raise SystemExit(f"error: {CONFIG.as_posix()}: unknown component {name!r}")
    order: list[str] = []
    for dependency in declared[name].get("depends", []):
        for resolved in _order(str(dependency), declared, (*trail, name)):
            if resolved not in order:
                order.append(resolved)
    order.append(name)
    return order


def load(root: pathlib.Path) -> dict[str, Component]:
    """Every declared component, with its effective rule list resolved."""
    with (root / CONFIG).open("rb") as file:
        source = tomllib.load(file)
    shared = _rules(source.get("shared", []), "shared")
    declared: dict[str, dict] = source.get("components", {})
    components: dict[str, Component] = {}
    for name, entry in declared.items():
        rules = list(shared)
        for member in _order(name, declared, ()):
            rules.extend(
                _rules(declared[member].get("paths", []), f"components.{member}")
            )
        components[name] = Component(
            name=name,
            description=str(entry.get("description", "")),
            depends=tuple(str(value) for value in entry.get("depends", [])),
            rules=tuple(rules),
        )
    return components


def state_tag(component: str) -> str:
    """The tag naming the commit a component last published from."""
    return f"{STATE_TAGS}/{component}"


def _git(root: pathlib.Path, *arguments: str) -> str:
    completed = subprocess.run(
        ("git", "-C", str(root), *arguments),
        capture_output=True,
        text=True,
    )
    if completed.returncode != 0:
        command = " ".join(arguments)
        raise SystemExit(f"error: git {command} failed: {completed.stderr.strip()}")
    return completed.stdout


def tracked(root: pathlib.Path) -> list[str]:
    """Every path Git tracks in the working tree."""
    return _git(root, "ls-files", "-z").split("\0")[:-1]


def entries(root: pathlib.Path, ref: str) -> list[Entry]:
    """Every object at `ref`. Submodule gitlinks appear as `commit` entries, so
    the vendored MapLibre Native pin is captured with no special case."""
    found = []
    for record in _git(root, "ls-tree", "-r", "-z", ref).split("\0")[:-1]:
        info, path = record.split("\t", 1)
        mode, _kind, oid = info.split(" ", 2)
        found.append(Entry(mode=mode, oid=oid, path=path))
    return found


def component_hash(component: Component, found: list[Entry]) -> str:
    """Digest the component's inputs at one commit.

    Raises `UncoveredPaths` when the commit holds a path the component does not
    classify, so a gate never runs against a partially classified tree.
    """
    lines = []
    uncovered = []
    for entry in found:
        verdict = classify(component.rules, entry.path)
        if verdict is None:
            uncovered.append(entry.path)
        elif verdict:
            lines.append(f"{entry.mode} {entry.path} {entry.oid}")
    if uncovered:
        raise UncoveredPaths(component, uncovered)
    digest = hashlib.sha256()
    for line in sorted(lines):
        digest.update(f"{line}\n".encode())
    return digest.hexdigest()


def describe(component: Component, paths: list[str]) -> str:
    """Name the component, the paths it leaves unclassified, and its rules."""
    shown = sorted(paths)
    listing = "\n".join(f"  {path}" for path in shown[:SAMPLE])
    if len(shown) > SAMPLE:
        listing += f"\n  ... and {len(shown) - SAMPLE} more"
    rules = "\n".join(f"  [{rule.origin}] {rule.text}" for rule in component.rules)
    return (
        f"component {component.name!r} leaves these paths unclassified:\n"
        f"{listing}\n"
        f"checked against:\n{rules}"
    )


def check_scopes(root: pathlib.Path) -> list[str]:
    """Report paths no rule classifies and rules no tracked path matches."""
    components = load(root)
    paths = tracked(root)
    problems = []

    for component in components.values():
        uncovered = [path for path in paths if classify(component.rules, path) is None]
        if uncovered:
            problems.append(describe(component, uncovered))

    seen: set[str] = set()
    for component in components.values():
        for rule in component.rules:
            if rule.pattern in seen:
                continue
            seen.add(rule.pattern)
            if not any(matches(rule.pattern, path) for path in paths):
                problems.append(f"[{rule.origin}] {rule.text} matches no tracked file")
    return problems
