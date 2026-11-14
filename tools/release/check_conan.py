#!/usr/bin/env python3
"""Resolve the generated Conan index and build a CMakeDeps consumer of one archive."""

from __future__ import annotations

import argparse
import os
import pathlib
import shutil
import subprocess
import tempfile

import conan_package
from distribution import Release
from native_package_release import sha256

ROOT = pathlib.Path(__file__).resolve().parents[2]


def check(archive: pathlib.Path) -> None:
    archive = archive.resolve(strict=True)
    with tempfile.TemporaryDirectory() as temporary:
        scratch = pathlib.Path(temporary)
        assets = scratch / "assets"
        assets.mkdir()
        # The recipe downloads the run's archive from its file URL and verifies
        # it exactly as it verifies a release's HTTPS download.
        (assets / "SHA256SUMS").write_text(f"{sha256(archive)}  {archive.name}\n")
        (assets / archive.name).symlink_to(archive)
        tree = scratch / "distribution"
        conan_package.write(tree, Release(assets), ROOT)
        consumer = scratch / "consumer"
        shutil.copytree(ROOT / "tests/distribution/conan", consumer)
        environment = os.environ | {"CONAN_HOME": str(scratch / "conan-home")}

        def run(*arguments: str) -> None:
            subprocess.run(arguments, cwd=consumer, env=environment, check=True)

        run("conan", "profile", "detect")
        run(
            "conan",
            "remote",
            "add",
            "distribution",
            str(tree),
            "--type=local-recipes-index",
        )
        run(
            "conan",
            "install",
            ".",
            "-r",
            "distribution",
            "--build=missing",
            "-of",
            "build",
            "-s",
            "build_type=Release",
        )
        run(
            "cmake",
            "-S",
            ".",
            "-B",
            "build",
            "-DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake",
            "-DCMAKE_BUILD_TYPE=Release",
        )
        run("cmake", "--build", "build", "--config", "Release")
        run(str(consumer / "build/consumer"))


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("archive", type=pathlib.Path)
    arguments = parser.parse_args()
    check(arguments.archive)


if __name__ == "__main__":
    main()
