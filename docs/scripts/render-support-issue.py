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
    "unknown": "❓",
}
NOT_APPLICABLE = "➖"
# A platform or architecture shows the furthest status any of its rows reached.
PRECEDENCE = ["tested", "build-only", "path-known", "blocked", "unknown"]
ARCHITECTURES = ["x64", "arm64", "arm", "riscv64", "loongarch64", "wasm32"]


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


def language_grid(matrix: dict[str, Any]) -> list[str]:
    platforms = matrix["platforms"]
    rows = []
    for index, language in enumerate(matrix["languages"]):
        row = [language["label"]]
        for platform in platforms:
            statuses = [
                backend["cells"][index]["status"]
                for arch in platform["architectures"]
                for backend in arch["backends"]
            ]
            row.append(ICONS[best(statuses)])
        rows.append(row)
    return table(["Language", *(p["label"] for p in platforms)], rows)


def architecture_grid(matrix: dict[str, Any]) -> list[str]:
    rows = []
    for platform in matrix["platforms"]:
        statuses: dict[str, list[str]] = {}
        for arch in platform["architectures"]:
            for backend in arch["backends"]:
                statuses.setdefault(arch["arch"], []).append(
                    backend["cells"][0]["status"]
                )
        rows.append(
            [platform["label"]]
            + [
                ICONS[best(statuses[arch])] if arch in statuses else NOT_APPLICABLE
                for arch in ARCHITECTURES
            ]
        )
    return table(["Platform", *ARCHITECTURES], rows)


def gap_list(matrix: dict[str, Any], status: str) -> list[str]:
    # Gaps that share a language and reason collapse into one line.
    grouped: dict[tuple[str, str, str], list[str]] = {}
    for gap in matrix["gaps"]:
        if gap["status"] == status:
            key = (gap["language"], gap["reason"], gap.get("link", ""))
            grouped.setdefault(key, []).append(gap["platform"])
    lines = []
    for (language, reason, link), platforms in grouped.items():
        line = f"- **{language} / {', '.join(platforms)}:** {reason}"
        if link:
            line += f" ([{link_label(link)}]({link}))"
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
            f"Each push to main rewrites this body from [`ci/workflow.toml`]"
            f"({SOURCE_ROOT}/ci/workflow.toml) and [`docs/support-gaps.toml`]"
            f"({SOURCE_ROOT}/docs/support-gaps.toml). Change those files, not "
            "this issue."
        ),
        "",
        f"{legend} · {NOT_APPLICABLE} does not apply",
        "",
        "A cell shows the furthest status of any architecture and backend in it.",
        "Kotlin covers Kotlin/JVM, Kotlin/Android, Kotlin/Native, and Kotlin/Wasm.",
        "",
        *language_grid(matrix),
        "",
        "The C library's architectures:",
        "",
        *architecture_grid(matrix),
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
