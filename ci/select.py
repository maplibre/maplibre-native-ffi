"""Ask Jev which coverage units a change puts at risk."""

from __future__ import annotations

import fnmatch
import hashlib
import json
import os
import pathlib
import re
import subprocess
import time
import tomllib
import urllib.error
import urllib.request

ROOT = pathlib.Path(__file__).resolve().parent.parent
CONFIG = pathlib.Path(__file__).with_name("select.toml")
ENDPOINT = "https://api.typesafe.ai/v1/systemone"
DEFAULT_MODEL = "jev-latest"
THRESHOLD = 0.5
# Bump when the change description sent to Jev changes shape.
INPUT_VERSION = 1

# One question sees the state and itself within 32k tokens. Code and diffs
# average well above three characters per token, so this leaves headroom.
STATE_BUDGET = 84_000
FILE_LIMIT = 24_000
# A larger change gets every unit: it costs many requests, and reviewers expect
# broad coverage for it anyway.
MAX_CHUNKS = 8
# The file list repeats in every chunk, so a huge change lists only its start.
FILE_LIST_LIMIT = 24_000
SUMMARY_FILE_LIMIT = 3_000
# Lockfiles and generated files matter only in summary: which packages moved.
SUMMARY_PATTERNS = re.compile(
    r"(\.lock|-lock\.ya?ml|lock\.json|Package\.resolved|\.lock\.json)$"
    r"|(^|/)generated/|\.g\.dart$|_generated\.|/gradle/verification-metadata\.xml$"
)


def load_config(path: pathlib.Path = CONFIG) -> dict:
    return tomllib.loads(path.read_text())


def git(*args: str) -> str:
    return subprocess.check_output(
        ["git", *args], cwd=ROOT, text=True, errors="replace"
    )


def split_patch(patch: str) -> list[tuple[str, str]]:
    """Split a unified diff into (path, text) per file."""
    parts = re.split(r"(?m)^(?=diff --git )", patch)
    files = []
    for part in parts:
        if not part.startswith("diff --git "):
            continue
        match = re.match(r"diff --git a/(.*?) b/(.*)", part)
        files.append((match.group(2) if match else "?", part))
    return files


def truncate(path: str, text: str) -> str:
    limit = SUMMARY_FILE_LIMIT if SUMMARY_PATTERNS.search(path) else FILE_LIMIT
    if text.startswith("diff --git") and "\ndeleted file mode" in text[:300]:
        limit = min(limit, SUMMARY_FILE_LIMIT)
    if len(text) <= limit:
        return text
    omitted = text[limit:].count("\n")
    return f"{text[:limit]}\n[... {omitted} more lines of this file's diff omitted]\n"


def submodule_commits(base: str, head: str, path: str) -> list[str]:
    """List upstream commit subjects between two submodule pins."""
    old, new = (
        git("rev-parse", f"{rev}:{path}").strip() if rev else "" for rev in (base, head)
    )
    url = git("config", "-f", ".gitmodules", f"submodule.{path}.url").strip()
    match = re.search(r"github\.com[:/](.+?)(\.git)?$", url)
    if not (old and new and match):
        return []
    try:
        output = subprocess.check_output(
            [
                "gh",
                "api",
                f"repos/{match.group(1)}/compare/{old}...{new}",
                "--jq",
                '.commits[].commit.message | split("\n")[0]',
            ],
            text=True,
            stderr=subprocess.DEVNULL,
        )
    except subprocess.CalledProcessError:
        return []
    return output.splitlines()


def describe_change(base: str, head: str, message: str) -> dict:
    """Collect the change as file summaries and patch chunks within budget."""
    status = {}
    for line in git("diff", "--name-status", "-M", base, head).splitlines():
        fields = line.split("\t")
        status[fields[-1]] = fields[0][0]
    stats = {}
    for line in git("diff", "--numstat", "-M", base, head).splitlines():
        added, deleted, path = line.split("\t", 2)
        if " => " in path:
            path = re.sub(r"\{(.*?) => (.*?)\}", r"\2", path).split(" => ")[-1]
        stats[path] = (added, deleted)
    files = [
        f"{kind} {path} (+{stats.get(path, ('?', '?'))[0]} -{stats.get(path, ('?', '?'))[1]})"
        for path, kind in status.items()
    ]

    submodules = []
    for line in git("diff", "--raw", base, head).splitlines():
        meta, path = line.split("\t", 1)
        if " 160000 " in f" {meta} ":
            submodules.append(
                {
                    "path": path,
                    "upstream_commits": submodule_commits(base, head, path)[:80],
                }
            )

    patches = [
        truncate(path, text)
        for path, text in split_patch(git("diff", "-M", base, head))
    ]
    listed, size = [], 0
    for line in files:
        size += len(line) + 4
        if size > FILE_LIST_LIMIT:
            listed.append(f"... and {len(files) - len(listed)} more changed files")
            break
        listed.append(line)
    change = {"message": message.strip()[:4000], "files": listed}
    if submodules:
        change["submodule_updates"] = submodules
    return {"change": change, "patches": patches}


def changed_paths(base: str, head: str) -> list[str]:
    return git("diff", "--name-only", "--no-renames", base, head).split("\n")[:-1]


def own_changes_base(base: str, fork: str) -> str:
    """Return a tree of `base` merged with the main commit `fork`.

    Diffing that tree against a head that contains `fork` shows the changes the
    pull request made since `base`, without main's changes merged in since.
    """
    if (
        subprocess.run(
            ["git", "merge-base", "--is-ancestor", fork, base], cwd=ROOT, check=False
        ).returncode
        == 0
    ):
        return base
    merged = subprocess.run(
        ["git", "merge-tree", "--write-tree", base, fork],
        cwd=ROOT,
        capture_output=True,
        text=True,
        check=False,
    )
    # On a conflict, the tree holds conflict markers where the pull request's
    # resolution differs, which is the interaction a reviewer would check.
    return merged.stdout.split()[0] if merged.returncode in (0, 1) else base


def forced_units(config: dict, paths: list[str]) -> dict[str, str]:
    """Map each unit with a path pattern matching a changed file to that pattern."""
    forced = {}
    for unit in config["units"]:
        for pattern in unit.get("paths", ()):
            if any(fnmatch.fnmatchcase(path, pattern) for path in paths):
                forced[unit["id"]] = pattern
                break
    return forced


def fingerprint(config: dict) -> str:
    """Identify everything that shapes Jev's answers, but not path patterns."""
    material = json.dumps(
        [
            config["overview"],
            questions(config),
            STATE_BUDGET,
            FILE_LIMIT,
            SUMMARY_FILE_LIMIT,
            SUMMARY_PATTERNS.pattern,
            FILE_LIST_LIMIT,
            MAX_CHUNKS,
            INPUT_VERSION,
        ]
    )
    return hashlib.sha256(material.encode()).hexdigest()[:12]


def chunk(patches: list[str], budget: int) -> list[str]:
    chunks, current = [], ""
    for text in patches:
        if current and len(current) + len(text) > budget:
            chunks.append(current)
            current = ""
        current += text[:budget]
    if current or not chunks:
        chunks.append(current)
    return chunks


def questions(config: dict) -> dict:
    result = {}
    for unit in config["units"]:
        relative = (
            ""
            if unit.get("standalone")
            else (
                " Platform-independent code that Linux x64 already tests does not "
                "need this coverage; select it when the change affects this "
                "coverage differently than Linux x64."
            )
        )
        result[unit["id"]] = {
            "type": "noul",
            "instructions": {
                "question": (
                    f"Should the `{unit['id']}` CI coverage run to verify this "
                    "change before merging? A maintainer reviewing the diff "
                    "would want to see it pass." + relative
                ),
                "coverage": " ".join(unit["covers"].split()),
            },
            "criteria": {
                "true": (
                    "The change modifies code, dependencies, tools, build "
                    "logic, packaging, or job definitions that this coverage "
                    "builds, installs, or tests in a way its result would verify."
                ),
                "false": (
                    "This coverage does not build, install, package, or test "
                    "anything the change modifies in a way that matters, or the "
                    "always-running hygiene check already verifies the change."
                ),
            },
        }
    return result


def ask(
    state: dict,
    question_map: dict,
    model: str,
    api_key: str,
    deadline: float | None = None,
) -> dict:
    body = json.dumps(
        {"model": model, "state": state, "questions": question_map}
    ).encode()
    for attempt in range(6):
        if deadline is not None and time.monotonic() > deadline:
            raise TimeoutError("Jev selection exceeded its deadline")
        request = urllib.request.Request(
            ENDPOINT,
            data=body,
            headers={
                "Authorization": f"Bearer {api_key}",
                "Content-Type": "application/json",
            },
        )
        try:
            with urllib.request.urlopen(request, timeout=120) as response:
                return json.load(response)
        except urllib.error.HTTPError as error:
            detail = error.read().decode(errors="replace")
            if error.code in (429, 500, 502, 503, 529) and attempt < 5:
                time.sleep(float(error.headers.get("retry-after") or 2**attempt))
                continue
            raise RuntimeError(f"Jev returned {error.code}: {detail}") from error
        # Connection failures and timeouts are both OSErrors.
        except OSError:
            if attempt < 5:
                time.sleep(2**attempt)
                continue
            raise
    raise AssertionError("unreachable")


def select(
    base: str,
    head: str,
    message: str,
    *,
    config: dict | None = None,
    model: str = DEFAULT_MODEL,
    api_key: str | None = None,
    deadline: float | None = None,
    cache: pathlib.Path | None = None,
    on_request=None,
) -> dict:
    """Return each unit's probability, the maximum across patch chunks.

    With a cache directory, an identical question reuses its earlier answer.
    """
    config = config or load_config()
    if cache:
        material = json.dumps([model, fingerprint(config), base, head, message])
        path = cache / f"{hashlib.sha256(material.encode()).hexdigest()[:24]}.json"
        if path.exists():
            return json.loads(path.read_text()) | {"input_tokens": 0}

    described = describe_change(base, head, message)
    question_map = questions(config)
    fixed = {"ci": " ".join(config["overview"].split()), "change": described["change"]}
    parts = chunk(described["patches"], STATE_BUDGET - len(json.dumps(fixed)))
    if len(parts) > MAX_CHUNKS:
        result = {"model": model, "units": dict.fromkeys(question_map, 1.0)}
        result |= {"oversized": True, "input_tokens": 0}
    else:
        api_key = api_key or os.environ["TYPESAFE_API_KEY"]
        responses = []
        for index, text in enumerate(parts):
            change = dict(described["change"], patch=text)
            if len(parts) > 1:
                change["patch_part"] = (
                    f"Part {index + 1} of {len(parts)} of the patch. The file "
                    "list covers the whole change."
                )
            state = dict(fixed, change=change)
            response = ask(state, question_map, model, api_key, deadline)
            if on_request:
                on_request({"state": state, "questions": question_map}, response)
            responses.append(response)
        result = {
            "model": responses[0]["model"],
            "units": {
                unit: max(response["answers"][unit]["noul"] for response in responses)
                for unit in question_map
            },
            "input_tokens": sum(r["usage"]["input_tokens"] for r in responses),
        }
    if cache:
        cache.mkdir(parents=True, exist_ok=True)
        path.write_text(json.dumps(result))
    return result
