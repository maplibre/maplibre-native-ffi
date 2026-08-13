#!/usr/bin/env python3
"""Regenerate the ClangSharp declarations the .NET binding builds against."""

from __future__ import annotations

import json
import os
import pathlib
import platform
import shutil
import subprocess
import tempfile

BINDING_DIR = pathlib.Path(__file__).resolve().parents[1]
ROOT = BINDING_DIR.parents[1]
MANIFEST = BINDING_DIR / ".config" / "dotnet-tools.json"
HEADER_DIR = ROOT / "include" / "maplibre_native_c"
OUTPUT_DIR = BINDING_DIR / "src" / "Maplibre.NativeFfi" / "Generated"

HEADERS = (
    "android",
    "base",
    "diagnostics",
    "logging",
    "runtime",
    "operation",
    "notification",
    "map",
    "camera",
    "projection",
    "query",
    "render_target",
    "render_session",
    "style",
    "surface",
    "texture",
)

# The generator loads libclang from the ClangSharp package built for the host,
# so the host maps to a .NET runtime identifier rather than a Python platform.
SYSTEMS = {"Darwin": "osx", "Linux": "linux", "Windows": "win"}
MACHINES = {"amd64": "x64", "x86_64": "x64", "aarch64": "arm64", "arm64": "arm64"}

NATIVE_LIBRARIES = {
    "osx": ("libclang.dylib", "libClangSharp.dylib"),
    "linux": ("libclang.so", "libClangSharp.so"),
    "win": ("libclang.dll", "libClangSharp.dll"),
}

# Those libraries sit outside the generator's own directory, so each platform's
# loader takes them from the variable it searches.
LIBRARY_PATH_VARIABLES = {
    "osx": "DYLD_LIBRARY_PATH",
    "linux": "LD_LIBRARY_PATH",
    "win": "PATH",
}


def host() -> tuple[str, str]:
    system = SYSTEMS.get(platform.system())
    machine = MACHINES.get(platform.machine().lower())
    if system is None or machine is None:
        raise SystemExit(
            "unsupported ClangSharp generator host: "
            f"{platform.system()}-{platform.machine()}"
        )
    return system, f"{system}-{machine}"


def generator_version() -> str:
    # The manifest is the one version of record, so a Dependabot bump carries
    # the generator and the native package it loads together.
    tools = json.loads(MANIFEST.read_text(encoding="utf-8"))["tools"]
    tool = tools.get("clangsharppinvokegenerator")
    if tool is None:
        raise SystemExit(f"{MANIFEST} declares no ClangSharp generator")
    return tool["version"]


def clang_include(version: str) -> pathlib.Path:
    clang = shutil.which("clang")
    if clang is not None:
        resource_dir = subprocess.run(
            (clang, "-print-resource-dir"),
            check=True,
            capture_output=True,
            text=True,
        ).stdout.strip()
        include = pathlib.Path(resource_dir) / "include"
        if include.is_dir():
            return include
    # A host without the Clang driver can still carry the resource headers, and
    # the generator's version tracks the LLVM release they belong to.
    major = version.split(".")[0]
    for directory in (f"/usr/lib/clang/{major}", f"/usr/lib64/clang/{major}"):
        include = pathlib.Path(directory) / "include"
        if include.is_dir():
            return include
    raise SystemExit(
        "Clang resource headers are unavailable; install the host Clang package"
    )


def native_directory(rid: str, version: str) -> pathlib.Path:
    packages = os.environ.get("NUGET_PACKAGES")
    root = (
        pathlib.Path(packages) if packages else pathlib.Path.home() / ".nuget/packages"
    )
    return root / f"clangsharppinvokegenerator.{rid}" / version / "tools/any" / rid


def generate(
    header: str, output: pathlib.Path, include: pathlib.Path, env: dict[str, str]
) -> None:
    source = HEADER_DIR / f"{header}.h"
    subprocess.run(
        (
            "dotnet",
            "tool",
            "run",
            "ClangSharpPInvokeGenerator",
            "--",
            "@scripts/generate-clangsharp.rsp",
            "-f",
            str(source),
            "-t",
            str(source),
            "-o",
            str(output),
            "-I",
            str(include),
        ),
        cwd=BINDING_DIR,
        env=env,
        check=True,
    )
    if not output.is_file() or output.stat().st_size == 0:
        raise SystemExit(f"ClangSharp produced no output for {header}.h")


def main() -> None:
    system, rid = host()
    version = generator_version()
    include = clang_include(version)

    subprocess.run(("dotnet", "tool", "restore"), cwd=BINDING_DIR, check=True)

    native_dir = native_directory(rid, version)
    for library in NATIVE_LIBRARIES[system]:
        if not (native_dir / library).is_file():
            raise SystemExit(
                f"missing ClangSharp native library: {native_dir / library}"
            )

    env = dict(os.environ)
    variable = LIBRARY_PATH_VARIABLES[system]
    search_path = env.get(variable)
    env[variable] = os.pathsep.join(filter(None, (str(native_dir), search_path)))

    # Generating into a staging directory keeps a failed run from leaving the
    # checked-in declarations half replaced.
    with tempfile.TemporaryDirectory() as temporary:
        staging = pathlib.Path(temporary)
        for header in HEADERS:
            generate(header, staging / f"{header}.g.cs", include, env)

        OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
        for stale in OUTPUT_DIR.glob("*.g.cs"):
            stale.unlink()
        for generated in staging.glob("*.g.cs"):
            shutil.move(generated, OUTPUT_DIR / generated.name)


if __name__ == "__main__":
    main()
