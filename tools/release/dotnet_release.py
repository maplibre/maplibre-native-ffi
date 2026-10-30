"""Pack the .NET packages from native archives, and smoke-test them on musl."""

from __future__ import annotations

import argparse
import os
import pathlib
import shutil
import subprocess
import sys
import tarfile
import tempfile

from binding_sbom import PACKAGE_SBOM
from binding_sbom import package as package_inventory
from native_package_release import package_presets

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))

from tools.ci.plan import dotnet_runtime_target as runtime_target

MANAGED_PROJECT = (
    ROOT / "bindings/dotnet/src/Maplibre.NativeFfi/Maplibre.NativeFfi.csproj"
)
RUNTIME_PROJECT = (
    ROOT
    / "bindings/dotnet/src/Maplibre.NativeFfi.Runtime/Maplibre.NativeFfi.Runtime.csproj"
)
PACKAGE_SMOKE_PROJECT = (
    ROOT
    / "bindings/dotnet/tests/Maplibre.NativeFfi.PackageSmoke/Maplibre.NativeFfi.PackageSmoke.csproj"
)
MUSL_SMOKE_VERSION = "0.0.0-musl-ci"
ANDROID_SMOKE_VERSION = "0.0.0-android-ci"
MUSL_RUNTIME_IMAGE = "mcr.microsoft.com/dotnet/runtime:10.0-alpine3.22"


def runtime_targets() -> dict[str, dict[str, str]]:
    """The presets of each runtime package, with their runtime identifiers."""
    targets: dict[str, dict[str, str]] = {}
    for preset in package_presets(ROOT):
        target = runtime_target(preset)
        if target is not None:
            backend, rid = target
            targets.setdefault(backend, {})[preset] = rid
    return targets


def run(
    *arguments: str,
    cwd: pathlib.Path = ROOT,
    environment: dict[str, str] | None = None,
) -> None:
    subprocess.run(arguments, cwd=cwd, env=environment, check=True)


def archive(input_dir: pathlib.Path, preset: str) -> pathlib.Path:
    matches = list(input_dir.glob(f"**/maplibre-native-c-{preset}.tar.gz"))
    if len(matches) != 1:
        raise SystemExit(f"expected one archive for {preset}, found {len(matches)}")
    return matches[0]


def dynamic_libraries(directory: pathlib.Path) -> list[pathlib.Path]:
    suffixes = (".so", ".dylib", ".dll")
    # A build tree's install also holds the test graphics fixtures, which no
    # runtime package carries, and a dSYM's symbol file takes its library's name
    # but doesn't load.
    return [
        path
        for path in directory.rglob("*")
        if path.is_file()
        and (path.name.endswith(suffixes) or ".so." in path.name)
        and "mln_test_graphics" not in path.name
        and not any(part.endswith(".dSYM") for part in path.parts)
    ]


def copy_runtime_assets(
    source: pathlib.Path, destination: pathlib.Path, preset: str
) -> None:
    libraries = dynamic_libraries(source)
    if not libraries:
        raise SystemExit(f"{preset} contains no dynamic libraries")
    destination.mkdir(parents=True, exist_ok=True)
    for library in libraries:
        shutil.copy2(library, destination / library.name)

    # The libraries bundle third-party code, so their license notices travel
    # with them.
    notices = next(source.glob("**/share/*/licenses"), None)
    if notices is None:
        raise SystemExit(f"{preset} carries no licenses")
    shutil.copytree(notices, destination / "licenses", dirs_exist_ok=True)


def pack(
    project: pathlib.Path,
    output: pathlib.Path,
    version: str,
    inventories: list[pathlib.Path] = (),
    **properties: str | pathlib.Path,
) -> None:
    name = "Maplibre.NativeFfi"
    dependencies = []
    if project == RUNTIME_PROJECT:
        name += f".Runtime.{properties['RenderBackend']}"
        dependencies = [f"Maplibre.NativeFfi@{version}"]
    sbom = output.parent / "sbom" / name / PACKAGE_SBOM
    package_inventory(
        name, version, "nuget", inventories, sbom, dependencies=dependencies
    )
    run(
        "dotnet",
        "pack",
        str(project),
        "--configuration",
        "Release",
        "--output",
        str(output),
        f"-p:PackageVersion={version}",
        f"-p:SbomPath={sbom.resolve()}",
        *(f"-p:{name}={value}" for name, value in properties.items()),
    )


def package(args: argparse.Namespace) -> None:
    args.output.mkdir(parents=True, exist_ok=True)
    # The mise tasks keep every dotnet output under build/, so packing does the
    # same rather than writing bin/ trees into the source projects.
    base_output = ROOT / "build" / "dotnet" / "package"
    pack(MANAGED_PROJECT, args.output, args.version, BaseOutputPath=f"{base_output}/")

    with tempfile.TemporaryDirectory() as temporary:
        staging = pathlib.Path(temporary)
        for backend, presets in runtime_targets().items():
            assets = staging / backend
            inventories = []
            for preset, rid in presets.items():
                extracted = staging / "extract" / preset
                with tarfile.open(archive(args.input, preset)) as tar:
                    tar.extractall(extracted, filter="data")
                copy_runtime_assets(extracted, assets / rid / "native", preset)
                inventories.append(
                    next(extracted.glob("*/share/maplibre-native-c/sbom.cdx.json"))
                )

            pack(
                RUNTIME_PROJECT,
                args.output,
                args.version,
                inventories=inventories,
                RenderBackend=backend,
                NativeAssetsDir=assets,
                BaseOutputPath=f"{base_output}/",
            )


def smoke_musl(args: argparse.Namespace) -> None:
    target = runtime_target(args.preset)
    if target is None or not args.preset.startswith("linux-musl-"):
        raise SystemExit(f"the musl package smoke cannot run {args.preset}")
    backend, rid = target
    work = ROOT / "build" / args.preset / "dotnet-musl-test"
    install = ROOT / "build" / args.preset / "install"
    packages = work / "packages"
    assets = work / "assets"
    publish = work / "publish"
    if work.exists():
        shutil.rmtree(work)
    packages.mkdir(parents=True)

    base_output = ROOT / "build" / "dotnet" / args.preset
    copy_runtime_assets(install, assets / rid / "native", args.preset)
    pack(
        MANAGED_PROJECT, packages, MUSL_SMOKE_VERSION, BaseOutputPath=f"{base_output}/"
    )
    pack(
        RUNTIME_PROJECT,
        packages,
        MUSL_SMOKE_VERSION,
        inventories=[install / "share/maplibre-native-c/sbom.cdx.json"],
        RenderBackend=backend,
        NativeAssetsDir=assets,
        BaseOutputPath=f"{base_output}/",
    )
    run(
        "dotnet",
        "publish",
        str(PACKAGE_SMOKE_PROJECT),
        "--configuration",
        "Release",
        "--runtime",
        rid,
        "--no-self-contained",
        "--output",
        str(publish),
        "--source",
        str(packages),
        "--source",
        "https://api.nuget.org/v3/index.json",
        f"-p:MaplibreNativeFfiPackageVersion={MUSL_SMOKE_VERSION}",
        # The smoke repacks a fixed version; restore from its fresh package directory
        # so a shared NuGet cache cannot substitute another revision's ABI.
        f"-p:RestorePackagesPath={work / 'restore'}",
        f"-p:MaplibreNativeFfiRenderBackend={backend}",
        f"-p:BaseOutputPath={base_output}/",
    )

    environment = os.environ.copy()
    environment["MISE_MONOREPO_ROOT"] = str(ROOT)
    environment["MLN_FFI_MUSL_TEST_IMAGE"] = MUSL_RUNTIME_IMAGE
    run(
        str(ROOT / "scripts/run-musl-test.sh"),
        args.preset,
        "dotnet",
        str(publish / "Maplibre.NativeFfi.PackageSmoke.dll"),
        environment=environment,
    )


def prepare_android(args: argparse.Namespace) -> None:
    target = runtime_target(args.preset)
    if target is None or not args.preset.startswith("android-"):
        raise SystemExit(f"the Android package test cannot build {args.preset}")
    backend, rid = target
    work = ROOT / "build" / args.preset / "dotnet-android-test"
    install = ROOT / "build" / args.preset / "install"
    if work.exists():
        shutil.rmtree(work)
    packages = work / "packages"
    packages.mkdir(parents=True)
    assets = work / "assets"
    base_output = ROOT / "build" / "dotnet" / args.preset
    copy_runtime_assets(install, assets / rid / "native", args.preset)
    pack(
        MANAGED_PROJECT,
        packages,
        ANDROID_SMOKE_VERSION,
        BaseOutputPath=f"{base_output}/",
    )
    pack(
        RUNTIME_PROJECT,
        packages,
        ANDROID_SMOKE_VERSION,
        inventories=[install / "share/maplibre-native-c/sbom.cdx.json"],
        RenderBackend=backend,
        NativeAssetsDir=assets,
        BaseOutputPath=f"{base_output}/",
    )


def main() -> None:
    parser = argparse.ArgumentParser()
    commands = parser.add_subparsers(dest="command", required=True)
    package_parser = commands.add_parser("package")
    package_parser.add_argument("input", type=pathlib.Path)
    package_parser.add_argument("output", type=pathlib.Path)
    package_parser.add_argument("version")
    smoke_parser = commands.add_parser("smoke-musl")
    smoke_parser.add_argument("preset")
    android_parser = commands.add_parser("prepare-android")
    android_parser.add_argument("preset")
    args = parser.parse_args()
    {
        "package": package,
        "smoke-musl": smoke_musl,
        "prepare-android": prepare_android,
    }[args.command](args)


if __name__ == "__main__":
    main()
