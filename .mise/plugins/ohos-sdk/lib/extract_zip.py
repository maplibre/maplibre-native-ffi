"""Extract a zip archive, keeping symbolic links and the executable bit.

mise's own extractor writes a symbolic link as a regular file holding its target,
and the SDK's compiler drivers are links, so the toolchain does not run without
them. `unzip` restores them, but it is absent from minimal Linux installs; Python
ships with everything that can host this SDK.
"""

from __future__ import annotations

import pathlib
import stat
import sys
import zipfile


def resolved(destination: pathlib.Path, name: str) -> pathlib.Path:
    """The path an entry writes to, rejecting one that escapes the destination."""
    target = (destination / name).resolve()
    if destination.resolve() not in target.parents:
        raise SystemExit(f"error: archive entry {name!r} escapes the destination")
    return target


def extract(archive: pathlib.Path, destination: pathlib.Path) -> None:
    with zipfile.ZipFile(archive) as bundle:
        for entry in bundle.infolist():
            target = resolved(destination, entry.filename)
            mode = entry.external_attr >> 16
            if stat.S_ISLNK(mode):
                target.parent.mkdir(parents=True, exist_ok=True)
                if target.is_symlink() or target.exists():
                    target.unlink()
                target.symlink_to(bundle.read(entry).decode())
                continue
            bundle.extract(entry, destination)
            # zip carries the mode out of band, so extract() leaves the default.
            # Directory modes stay as they are, because a read-only one would stop
            # the entries below it from being written.
            if not entry.is_dir() and mode & 0o777:
                target.chmod(mode & 0o777)


def main(argv: list[str]) -> None:
    if len(argv) != 3:
        raise SystemExit(f"usage: {pathlib.Path(argv[0]).name} <archive> <destination>")
    extract(pathlib.Path(argv[1]), pathlib.Path(argv[2]))


if __name__ == "__main__":
    main(sys.argv)
