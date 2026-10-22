#!/usr/bin/env python3
"""Writes the release version the docs quote into src/generated/versions.json."""

import json
import pathlib
import sys

REPO_ROOT = pathlib.Path(__file__).resolve().parents[2]
OUTPUT_PATH = REPO_ROOT / "docs" / "src" / "generated" / "versions.json"
sys.path.insert(0, str(REPO_ROOT))

from ci.release_version import release_tags


def main() -> int:
    tags = release_tags(repository=REPO_ROOT)
    if not tags:
        raise SystemExit(
            "the checkout has no release tags; fetch tags to build the docs"
        )
    # Pages quote the latest release as `{{version}}`.
    versions = {"version": str(max(tags.values()))}
    OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT_PATH.write_text(json.dumps(versions, indent=2) + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
