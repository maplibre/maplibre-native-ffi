#!/usr/bin/env python3
import json
import pathlib
import re
from typing import Any

REPO_ROOT = pathlib.Path(__file__).resolve().parents[2]
MATRIX_PATH = REPO_ROOT / "docs" / "src" / "generated" / "support-matrix.json"
SOURCE_ROOT = "https://github.com/maplibre/maplibre-native-ffi/blob/main"
MATRIX_URL = "https://maplibre.org/maplibre-native-ffi/#status"

ICONS = {
    "tested": "✅",
    "build-only": "🔨",
    "path-known": "⬜",
    "blocked": "⛔",
    "not-investigated": "❓",
    "not-applicable": "➖",
}
# A platform shows the furthest status any of its rows reached.
PRECEDENCE = [
    "tested",
    "build-only",
    "path-known",
    "blocked",
    "not-investigated",
    "not-applicable",
]


def best(statuses: list[str]) -> str:
    return min(statuses, key=PRECEDENCE.index)


def link_label(url: str) -> str:
    patterns = [
        (r"github\.com/([^/]+/[^/]+)/(?:issues|pull)/(\d+)", r"\1#\2"),
        (r"codeberg\.org/([^/]+/[^/]+)/(?:issues|pulls)/(\d+)", r"\1#\2"),
        (r"youtrack\.jetbrains\.com/issue/([A-Z]+-\d+)", r"\1"),
        (r"peps\.python\.org/pep-0*(\d+)", r"PEP \1"),
        (r"github\.com/([^/]+/[^/]+)", r"\1"),
    ]
    for pattern, label in patterns:
        match = re.search(pattern, url)
        if match:
            return match.expand(label)
    return re.sub(r"^https?://(www\.)?", "", url).split("/")[0]


def table(header: list[str], rows: list[list[str]]) -> list[str]:
    return [
        "| " + " | ".join(header) + " |",
        "| " + " | ".join("---" for _ in header) + " |",
        *("| " + " | ".join(row) + " |" for row in rows),
    ]


def binding_grid(matrix: dict[str, Any]) -> list[str]:
    bindings = matrix["bindings"]
    rows = []
    for platform in matrix["platforms"]:
        row = [platform["label"]]
        for index in range(len(bindings)):
            statuses = [
                backend["cells"][index]["status"]
                for arch in platform["architectures"]
                for backend in arch["backends"]
            ]
            row.append(ICONS[best(statuses)])
        rows.append(row)
    return table(["Platform", *(binding["label"] for binding in bindings)], rows)


def gap_list(matrix: dict[str, Any], status: str) -> list[str]:
    lines = []
    for gap in matrix["gaps"]:
        if gap["status"] != status:
            continue
        line = f"- **{gap['binding']} on {gap['scope']}:** {gap['reason']}"
        if "link" in gap:
            line += f" ([{link_label(gap['link'])}]({gap['link']}))"
        lines.append(line)
    return lines


def render(matrix: dict[str, Any]) -> str:
    legend = " · ".join(
        f"{ICONS[row['status']]} {row['label'][0].lower()}{row['label'][1:]}"
        for row in matrix["statuses"]
    )
    lines = [
        (
            f"The [support matrix]({MATRIX_URL}) shows every platform, "
            "architecture, and backend. This issue summarizes it and tracks the "
            "gaps."
        ),
        "",
        (
            f"Each push to main rewrites this body from [`CMakePresets.json`]"
            f"({SOURCE_ROOT}/CMakePresets.json), [`tools/ci/suites.py`]"
            f"({SOURCE_ROOT}/tools/ci/suites.py), and [`docs/support-gaps.toml`]"
            f"({SOURCE_ROOT}/docs/support-gaps.toml). Change those files, not "
            "this issue."
        ),
        "",
        legend,
        "",
        (
            "A cell shows the furthest status of any architecture and backend on "
            "its platform. TypeScript runs under Node, Bun, and Deno, in ArkTS "
            "apps on OpenHarmony, in the browser, and in embedded Hermes on "
            "Android, iOS, Mac Catalyst, tvOS, and visionOS."
        ),
        "",
        *binding_grid(matrix),
        "",
        "## Blocked",
        "",
        *gap_list(matrix, "blocked"),
        "",
        "## Known paths",
        "",
        *gap_list(matrix, "path-known"),
    ]
    return "\n".join(lines) + "\n"


def main() -> None:
    matrix = json.loads(MATRIX_PATH.read_text(encoding="utf-8"))
    print(render(matrix), end="")


if __name__ == "__main__":
    main()
