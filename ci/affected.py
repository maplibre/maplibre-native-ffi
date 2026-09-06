"""Read mise's affected project graph without executing tasks or dependencies."""

from __future__ import annotations

import json
import os
import pathlib
import re
import subprocess

from ci.workflow import consumer_roots, load_configuration, preset_sets


def mise_json(root: pathlib.Path, *args: str) -> dict:
    result = subprocess.run(
        ["mise", *args],
        cwd=root,
        env={**os.environ, "MISE_AUTO_INSTALL": "0", "MISE_NO_HOOKS": "1"},
        check=True,
        capture_output=True,
        text=True,
        timeout=60,
    )
    value = json.loads(result.stdout)
    if not isinstance(value, dict):
        raise TypeError("mise must return a JSON object")
    return value


def projects(value: dict) -> dict[str, str]:
    """Validate the fields shared by graph and affected-selection JSON."""
    entries = value.get("projects")
    if not isinstance(entries, list):
        raise TypeError("mise output has no project list")
    result = {}
    for entry in entries:
        if not isinstance(entry, dict):
            raise TypeError("Invalid mise project")
        name, root = entry.get("id"), entry.get("root")
        if not isinstance(name, str) or not name or name in result:
            raise ValueError("Invalid or duplicate mise project ID")
        if not isinstance(root, str):
            raise TypeError(f"Invalid mise project root for {name}")
        # Mise serializes PathBuf roots with the host's path separators.
        root = root.replace("\\", "/")
        if (
            not root
            or pathlib.PurePosixPath(root).is_absolute()
            or pathlib.PureWindowsPath(root).drive
            or ".." in pathlib.PurePosixPath(root).parts
            or str(pathlib.PurePosixPath(root)) != root
        ):
            raise ValueError(f"Invalid mise project root for {name}")
        result[name] = root
    return result


def check_graph(root: pathlib.Path) -> dict[str, str]:
    graph = mise_json(root, "tasks", "graph", "--json")
    known = projects(graph)
    for project in graph["projects"]:
        dependencies = project.get("dependencies")
        if not isinstance(dependencies, list) or any(
            not isinstance(name, str) or name not in known for name in dependencies
        ):
            raise ValueError(f"Invalid mise dependencies for {project['id']}")
    source, presets = load_configuration(root)
    required = {".", "docs"}
    for preset in preset_sets(presets)[0]:
        required.update(consumer_roots(source, preset))
    missing = required - set(known.values())
    if missing:
        raise ValueError(f"CI task roots missing from mise graph: {sorted(missing)}")
    return known


def affected_roots(root: pathlib.Path, base: str, head: str) -> set[str]:
    if not all(
        isinstance(sha, str) and re.fullmatch(r"[0-9a-f]{40}", sha)
        for sha in (base, head)
    ):
        raise ValueError("Affected selection requires explicit Git commit SHAs")
    known = check_graph(root)
    # --affected-json exits before task execution and prerequisite expansion.
    # The adapter consumes projects; ** also includes colon-nested task names.
    selection = mise_json(
        root,
        "run",
        "--affected",
        "--affected-json",
        "--affected-base",
        base,
        "--affected-head",
        head,
        "**",
    )
    if selection.get("base") != base or selection.get("head") != head:
        raise ValueError("mise compared unexpected Git revisions")
    affected = projects(selection)
    if any(known.get(name) != path for name, path in affected.items()):
        raise ValueError("Affected projects disagree with the mise graph")
    return set(affected.values())
