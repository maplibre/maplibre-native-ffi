#!/usr/bin/env python3
"""Writes the release versions the docs quote into src/generated/versions.json."""

import json
import pathlib
import sys

REPO_ROOT = pathlib.Path(__file__).resolve().parents[2]
OUTPUT_PATH = REPO_ROOT / "docs" / "src" / "generated" / "versions.json"
sys.path.insert(0, str(REPO_ROOT))

from ci.release_version import family, release_tags

# Each key names a release family, and pages quote it as `{{key}}`.
FAMILIES = ("kotlin",)


def latest_release(name: str) -> str:
    tags = release_tags(family(name), repository=REPO_ROOT)
    if not tags:
        raise SystemExit(
            f"the checkout has no {name} release tags; fetch tags to build the docs"
        )
    return str(max(tags.values()))


def main() -> int:
    versions = {name: latest_release(name) for name in FAMILIES}
    OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT_PATH.write_text(json.dumps(versions, indent=2) + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
