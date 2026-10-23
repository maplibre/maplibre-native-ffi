#!/usr/bin/env python3
"""Prepares the Rust crates for publication at a release tag."""

from __future__ import annotations

import argparse
import pathlib
import re

from native_package_release import write_pin
from release_version import parse_tag

SYS_CRATE = pathlib.Path("bindings/rust/crates/maplibre-native-ffi-sys")
PIN_PATH = SYS_CRATE / "native_release.json"


def substitute(path: pathlib.Path, pattern: str, replacement: str, count: int) -> None:
    text, found = re.subn(pattern, replacement, path.read_text(), flags=re.MULTILINE)
    if found != count:
        raise SystemExit(f"{path} has {found} matches of {pattern!r}; expected {count}")
    path.write_text(text, encoding="utf-8")


def prepare(
    workspace: pathlib.Path, tag: str, checksums: pathlib.Path | None = None
) -> None:
    _, version = parse_tag(tag)

    write_pin(workspace / PIN_PATH, tag, checksums)

    # The workspace version, then each published crate's requirement on the
    # others, which ship in lockstep.
    manifest = workspace / "Cargo.toml"
    substitute(manifest, r'^version = "0\.0\.0"$', f'version = "{version}"', 1)
    substitute(
        manifest,
        r'^(maplibre-native-ffi-(?:core|sys) = \{.*version = )"=0\.0\.0"',
        rf'\1"={version}"',
        2,
    )

    # The platform crate that Emscripten links from Cargo is not published, so
    # the published sys crate leaves the dependency out.
    substitute(
        workspace / SYS_CRATE / "Cargo.toml",
        r"""^\[target\.'cfg\(target_os = "emscripten"\)'\.dependencies\]\nmln-ffi-platform = .*\n""",
        "",
        1,
    )


def main(arguments: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("workspace", type=pathlib.Path)
    parser.add_argument("tag")
    parser.add_argument(
        "--checksums",
        type=pathlib.Path,
        help="A SHA256SUMS to pin, rather than the release's",
    )
    args = parser.parse_args(arguments)
    prepare(args.workspace, args.tag, args.checksums)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
