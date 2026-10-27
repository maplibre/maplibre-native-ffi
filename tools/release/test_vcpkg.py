#!/usr/bin/env python3
"""Install the generated Git registry's port and build a CMake consumer.

The assets directory holds this run's native archive. vcpkg reads that archive
through a file URL, with the same hash and extraction path as a release URL.
"""

from __future__ import annotations

import argparse
import os
import pathlib
import shutil
import subprocess
import tempfile

import distribution
from vcpkg_registry import PORT, write_json


def run(*arguments: str | pathlib.Path, **kwargs: object) -> None:
    subprocess.run(arguments, check=True, **kwargs)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("assets", type=pathlib.Path)
    parser.add_argument("vcpkg", type=pathlib.Path)
    parser.add_argument("--triplet", required=True)
    parser.add_argument(
        "--backend", required=True, choices=("opengl", "metal", "vulkan")
    )
    args = parser.parse_args()
    with tempfile.TemporaryDirectory() as temporary:
        root = pathlib.Path(temporary)
        registry = root / "registry"
        distribution.write(registry, distribution.Release(args.assets.resolve()))
        run("git", "init", "--quiet", registry)
        run("git", "-C", registry, "add", "--all")
        run(
            "git",
            "-C",
            registry,
            "-c",
            "user.name=Registry test",
            "-c",
            "user.email=registry@example.invalid",
            "-c",
            "commit.gpgSign=false",
            "commit",
            "--quiet",
            "-m",
            "Registry fixture",
        )
        baseline = subprocess.check_output(
            ["git", "-C", registry, "rev-parse", "HEAD"], text=True
        ).strip()
        consumer = root / "consumer"
        shutil.copytree(distribution.ROOT / "tests/package", consumer)
        write_json(
            consumer / "vcpkg.json",
            {
                "name": "registry-consumer",
                "version-string": "0",
                "dependencies": [{"name": PORT, "features": [args.backend]}],
            },
        )
        write_json(
            consumer / "vcpkg-configuration.json",
            {
                "default-registry": None,
                "registries": [
                    {
                        "kind": "git",
                        "repository": registry.as_uri(),
                        "baseline": baseline,
                        "packages": [PORT],
                    }
                ],
            },
        )
        build = root / "build"
        run(
            "cmake",
            "-S",
            consumer,
            "-B",
            build,
            "-G",
            "Ninja",
            f"-DCMAKE_TOOLCHAIN_FILE={args.vcpkg.resolve()}/scripts/buildsystems/vcpkg.cmake",
            f"-DVCPKG_TARGET_TRIPLET={args.triplet}",
            "-DCMAKE_BUILD_TYPE=Release",
            env=os.environ | {"VCPKG_BINARY_SOURCES": "clear"},
        )
        run("cmake", "--build", build)
        run(build / "shared_consumer")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
