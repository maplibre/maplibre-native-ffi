#!/usr/bin/env python3
"""Builds and runs an xmake consumer against a local native release archive."""

from __future__ import annotations

import argparse
import os
import pathlib
import shutil
import subprocess
import tempfile

from distribution import Release
from xmake_package import write

ROOT = pathlib.Path(__file__).resolve().parents[2]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("archive", type=pathlib.Path)
    parser.add_argument("--backend", default="default")
    args = parser.parse_args()
    archive = args.archive.resolve(strict=True)
    with tempfile.TemporaryDirectory() as temporary:
        scratch = pathlib.Path(temporary)
        assets = scratch / "assets"
        assets.mkdir()
        shutil.copyfile(archive, assets / archive.name)
        tree = scratch / "repository"
        write(tree, Release(assets), ROOT)
        consumer = scratch / "consumer"
        consumer.mkdir()
        shutil.copyfile(ROOT / "tests/package/main.c", consumer / "main.c")
        (consumer / "xmake.lua").write_text(
            f'add_repositories("maplibre {tree.as_posix()}")\n'
            'add_requires("maplibre-native-c 0.0.0", '
            f'{{system = false, configs = {{backend = "{args.backend}"}}}})\n'
            'target("consumer")\n'
            '    set_kind("binary")\n'
            '    add_files("main.c")\n'
            '    add_packages("maplibre-native-c")\n',
            encoding="utf-8",
        )
        environment = os.environ | {"XMAKE_GLOBALDIR": str(scratch / "global")}
        for command in (["xmake", "f", "-y"], ["xmake", "-v"], ["xmake", "run"]):
            subprocess.run(command, cwd=consumer, env=environment, check=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
