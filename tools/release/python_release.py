#!/usr/bin/env python3
"""Build dependency metadata and version Python packages for publication.

Runtime wheels carry their native notices at build time. Release and snapshot
packing changes their distribution metadata while preserving the tested binary.
"""

from __future__ import annotations

import argparse
import json
import pathlib
import re
import shutil
import subprocess
import tempfile
import zipfile

from release_version import parse_version
from sbom import (
    REPOSITORY,
    SBOM_NAME,
    cargo_components,
    cargo_notices,
    component,
    document,
    package_inventory,
    stamp,
    without_native_rust,
    write_json,
)

BACKENDS = ("opengl", "metal", "vulkan")
DESCRIPTOR_PATH = pathlib.Path("share/maplibre-native-c/artifact.json")
LICENSE = pathlib.Path(__file__).resolve().parents[2] / "LICENSE"
DEV_SUFFIX = re.compile(r"\.dev[0-9]+\Z")


def substitute(path: pathlib.Path, pattern: str, replacement: str) -> None:
    text, found = re.subn(pattern, replacement, path.read_text(), flags=re.MULTILINE)
    if found != 1:
        raise SystemExit(f"{path} has {found} matches of {pattern!r}; expected 1")
    path.write_text(text, encoding="utf-8")


def prepare(package: pathlib.Path, version: str) -> None:
    # A snapshot builds toward the next release, as <release>.devSTAMP.
    parse_version(DEV_SUFFIX.sub("", version))
    substitute(
        package / "pyproject.toml", r'^version = "0\.0\.0"$', f'version = "{version}"'
    )
    shutil.copy2(LICENSE, package / "LICENSE")


def runtime_metadata(package: pathlib.Path, prefix: pathlib.Path) -> None:
    descriptor = json.loads((prefix / DESCRIPTOR_PATH).read_text())
    backend = descriptor["backend"]
    if backend not in BACKENDS:
        raise SystemExit(f"no Python runtime serves the {backend} backend")
    # The wheel embeds the native library, so it carries the notices of the
    # third-party code in it.
    notices = next(prefix.glob("share/*/licenses"), None)
    if notices is None:
        raise SystemExit(f"{prefix} carries no licenses")
    licenses = package / "runtimes" / backend / "licenses"
    shutil.rmtree(licenses, ignore_errors=True)
    shutil.copytree(notices, licenses)
    shutil.copy2(LICENSE, licenses.parent / "LICENSE")
    target = descriptor["platform"]["rustTarget"]
    manifest = package / "Cargo.toml"
    flags = ("--features", "pyo3/extension-module")
    if descriptor["platform"]["id"].startswith("android-"):
        flags += ("--no-default-features",)
    with tempfile.TemporaryDirectory() as temporary:
        about_path = pathlib.Path(temporary) / "cargo-about.json"
        subprocess.run(
            [
                "cargo",
                "about",
                "generate",
                "--manifest-path",
                str(manifest),
                "--config",
                str(REPOSITORY / "src/platform/rust/about.toml"),
                "--target",
                target,
                "--locked",
                "--fail",
                "--format",
                "json",
                "--output-file",
                str(about_path),
                *flags,
            ],
            cwd=REPOSITORY,
            check=True,
        )
        about = json.loads(about_path.read_text(encoding="utf-8"))
    crates, graph, root_ref = cargo_components(about, target, manifest, flags)
    rust_version = subprocess.check_output(["rustc", "--version"], text=True).split()[1]
    rust = component("rust-std", rust_version, "MIT AND Apache-2.0")
    crates.append(rust)
    graph[root_ref].add(rust["bom-ref"])
    crate_root = next(item for item in crates if item["bom-ref"] == root_ref)
    extension = document(
        crate_root, [item for item in crates if item is not crate_root], graph
    )
    inventory = json.loads((prefix / "share/maplibre-native-c" / SBOM_NAME).read_text())
    result = package_inventory(
        f"maplibre-native-ffi-runtime-{backend}",
        "0.0.0",
        "pypi",
        [without_native_rust(inventory), extension],
    )
    result["metadata"]["component"]["properties"] = [
        {"name": "maplibre:source-commit", "value": descriptor["commit"]}
    ]
    write_json(licenses.parent / SBOM_NAME, result)
    (licenses / "rust.md").write_text(cargo_notices(about), encoding="utf-8")
    sysroot = pathlib.Path(
        subprocess.check_output(["rustc", "--print", "sysroot"], text=True).strip()
    )
    shutil.copyfile(
        sysroot / "share/doc/rust/COPYRIGHT-library.html", licenses / "rust-std.html"
    )


def embed_wheel(wheel: pathlib.Path, inventory: dict) -> None:
    from wheel.wheelfile import WheelFile

    distribution, version, _ = wheel.name.split("-", 2)
    info = f"{distribution}-{version}.dist-info/"
    sbom = info + "sboms/maplibre-native-ffi.cdx.json"
    with tempfile.TemporaryDirectory(dir=wheel.parent) as temporary:
        destination = pathlib.Path(temporary) / wheel.name
        with zipfile.ZipFile(wheel) as source, WheelFile(destination, "w") as target:
            for entry in source.infolist():
                # Maturin's Cargo-only inventory includes build dependencies and
                # local Cargo IDs. Replace it with the composed runtime inventory.
                if entry.filename != info + "RECORD" and not entry.filename.startswith(
                    info + "sboms/"
                ):
                    target.writestr(entry, source.read(entry))
            target.writestr(sbom, json.dumps(inventory, indent=2) + "\n")
        destination.replace(wheel)


def embed(wheels: pathlib.Path, inventory_path: pathlib.Path) -> None:
    inventory = json.loads(inventory_path.read_text())
    distribution = inventory["metadata"]["component"]["name"].replace("-", "_")
    for wheel in sorted(wheels.glob(f"{distribution}-*.whl")):
        embed_wheel(wheel, inventory)


def embed_api(wheels: pathlib.Path) -> None:
    for wheel in sorted(wheels.glob("maplibre_native_ffi-*.whl")):
        _, version, _ = wheel.name.split("-", 2)
        embed_wheel(
            wheel, package_inventory("maplibre-native-ffi", version, "pypi", [])
        )


def pack(wheels: pathlib.Path, output: pathlib.Path, version: str) -> None:
    # WheelFile regenerates RECORD after release metadata changes.
    from wheel.wheelfile import WheelFile

    parse_version(DEV_SUFFIX.sub("", version))
    inputs = sorted(wheels.rglob("*.whl"))
    if not inputs:
        raise SystemExit(f"no runtime wheels in {wheels}")
    output.mkdir(parents=True, exist_ok=True)
    for wheel in inputs:
        distribution, old_version, tags = wheel.name.split("-", 2)
        old_info = f"{distribution}-{old_version}.dist-info/"
        new_info = f"{distribution}-{version}.dist-info/"
        destination = output / f"{distribution}-{version}-{tags}"
        with zipfile.ZipFile(wheel) as source, WheelFile(destination, "w") as target:
            source.getinfo(old_info + "sboms/maplibre-native-ffi.cdx.json")
            for entry in source.infolist():
                if entry.filename == old_info + "RECORD":
                    continue
                data = source.read(entry)
                if entry.filename == old_info + "METADATA":
                    for field in ("Version: ", "Requires-Dist: maplibre-native-ffi=="):
                        old = f"{field}{old_version}\n".encode()
                        if old not in data:
                            raise SystemExit(f"{wheel} has no matching {field.strip()}")
                        data = data.replace(old, f"{field}{version}\n".encode(), 1)
                if entry.filename == old_info + "sboms/maplibre-native-ffi.cdx.json":
                    inventory = json.loads(data)
                    # CI generated this inventory from the binaries in this wheel.
                    # Publishing changes their version, not their source revision.
                    commit = next(
                        prop["value"]
                        for prop in inventory["metadata"]["component"]["properties"]
                        if prop["name"] == "maplibre:source-commit"
                    )
                    data = (
                        json.dumps(stamp(inventory, version, commit), indent=2) + "\n"
                    ).encode()
                entry.filename = entry.filename.replace(old_info, new_info, 1)
                # WheelFile regenerates RECORD, including each file's hash and size.
                target.writestr(entry, data)


def main(arguments: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    commands = parser.add_subparsers(dest="command", required=True)
    metadata_parser = commands.add_parser("metadata")
    metadata_parser.add_argument("package", type=pathlib.Path)
    metadata_parser.add_argument("prefix", type=pathlib.Path)
    embed_parser = commands.add_parser("embed")
    embed_parser.add_argument("wheels", type=pathlib.Path)
    embed_parser.add_argument("inventory", type=pathlib.Path)
    api_parser = commands.add_parser("embed-api")
    api_parser.add_argument("wheels", type=pathlib.Path)
    pack_parser = commands.add_parser("pack")
    pack_parser.add_argument("package", type=pathlib.Path)
    pack_parser.add_argument("version")
    pack_parser.add_argument("wheels", type=pathlib.Path)
    args = parser.parse_args(arguments)
    if args.command == "metadata":
        runtime_metadata(args.package.resolve(), args.prefix)
    elif args.command == "embed":
        embed(args.wheels, args.inventory)
    elif args.command == "embed-api":
        embed_api(args.wheels)
    else:
        prepare(args.package, args.version)
        pack(args.wheels, args.package / "dist", args.version)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
