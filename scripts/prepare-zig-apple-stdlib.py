#!/usr/bin/env python3
"""Backport Zig #37063 for Apple mobile test binaries.

Zig 0.17.0's threaded I/O table references Darwin process spawning on targets
that cannot spawn. Keep the installed toolchain intact and patch a local overlay.
Remove this workaround when the pinned Zig release includes the upstream fix:
https://codeberg.org/ziglang/zig/pulls/37063
"""

import argparse
from pathlib import Path


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("zig_lib_dir", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    source = args.zig_lib_dir.resolve()
    original = "if (is_darwin) if (t.spawnDarwin("
    replacement = "if (is_darwin and process.can_spawn) if (t.spawnDarwin("
    threaded = (source / "std/Io/Threaded.zig").read_text()
    if threaded.count(original) != 1:
        raise SystemExit("The Zig #37063 backport no longer matches the pinned stdlib")

    # Share the toolchain files through symlinks; only Threaded.zig is rewritten.
    for relative, replaced in (("", "std"), ("std", "Io"), ("std/Io", "Threaded.zig")):
        destination = args.output / relative
        destination.mkdir(parents=True, exist_ok=True)
        for entry in (source / relative).iterdir():
            if entry.name != replaced:
                link = destination / entry.name
                if not link.is_symlink():
                    link.symlink_to(entry, target_is_directory=entry.is_dir())
    (args.output / "std/Io/Threaded.zig").write_text(
        threaded.replace(original, replacement, 1)
    )


if __name__ == "__main__":
    main()
