#!/usr/bin/env python3
"""Pin the core release that the published Dart package downloads.

The Dart bindings are generated from the checkout's C headers, so the pinned
core release must be tagged on the checked-out commit.
"""

from __future__ import annotations

import argparse
import json
import pathlib
import subprocess
import urllib.request

RELEASE_URL = "https://github.com/maplibre/maplibre-native-ffi/releases/download"


def commit(revision: str) -> str:
    return subprocess.run(
        ["git", "rev-parse", f"{revision}^{{commit}}"],
        check=True,
        stdout=subprocess.PIPE,
        text=True,
    ).stdout.strip()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("tag", help="a core release tag, such as core/v0.202610.1")
    parser.add_argument("output", type=pathlib.Path)
    arguments = parser.parse_args()

    if not arguments.tag.startswith("core/v"):
        parser.error(f"{arguments.tag} is not a core release tag")
    if commit(arguments.tag) != commit("HEAD"):
        raise SystemExit(f"{arguments.tag} does not point at the checked-out commit")

    with urllib.request.urlopen(
        f"{RELEASE_URL}/{arguments.tag}/SHA256SUMS"
    ) as response:
        checksums = response.read().decode()
    archives = {
        name: digest
        for digest, name in (line.split("  ", 1) for line in checksums.splitlines())
        if name.startswith("maplibre-native-c-") and name.endswith(".tar.gz")
    }
    arguments.output.write_text(
        json.dumps({"tag": arguments.tag, "sha256": archives}, indent=2, sort_keys=True)
        + "\n",
        encoding="utf-8",
    )
    print(f"Pinned {arguments.tag} in {arguments.output}.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
