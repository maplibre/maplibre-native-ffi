"""Compose the inventories of registry packages."""

from __future__ import annotations

import argparse
import json
import pathlib
import re
import shutil
from collections.abc import Sequence
from urllib.parse import quote

from sbom import (
    REPOSITORY,
    SBOM_NAME,
    component,
    document,
    git,
    package_inventory,
    stamp,
    write_json,
)

PACKAGE_SBOM = "maplibre-native-ffi.cdx.json"


def npm_inventory(directory: pathlib.Path, notices: pathlib.Path) -> dict:
    components = {}
    graph = {}

    def visit(directory: pathlib.Path) -> str:
        directory = directory.resolve()
        manifest = json.loads((directory / "package.json").read_text())
        name, version = manifest["name"], manifest["version"]
        item = component(name, version, manifest["license"])
        item["purl"] = f"pkg:npm/{quote(name, safe='/')}@{quote(version, safe='')}"
        ref = item["bom-ref"] = item["purl"]
        if ref in components:
            return ref
        components[ref] = item
        graph[ref] = set()
        for notice in sorted(
            {*directory.glob("LICENSE*"), *directory.glob("CopyrightNotice*")}
        ):
            shutil.copyfile(notice, notices / f"{name.replace('/', '_')}-{notice.name}")
        for dependency in manifest.get("dependencies", {}):
            dependency_dir = next(
                parent / "node_modules" / dependency
                for parent in [directory, *directory.parents]
                if (parent / "node_modules" / dependency / "package.json").is_file()
            )
            graph[ref].add(visit(dependency_dir))
        return ref

    root = visit(directory)
    return document(
        components[root],
        [item for ref, item in components.items() if ref != root],
        graph,
    )


def package(
    name: str,
    version: str,
    ecosystem: str,
    inputs: list[pathlib.Path],
    output: pathlib.Path,
    npm: Sequence[pathlib.Path] = (),
    dependencies: Sequence[str] = (),
    optional_dependencies: Sequence[str] = (),
    classifier: str = "",
) -> dict:
    inventories = [json.loads(path.read_text()) for path in inputs]
    for index, inventory in enumerate(inventories):
        commit = next(
            p["value"]
            for p in inventory["metadata"]["component"]["properties"]
            if p["name"] == "maplibre:source-commit"
        )
        inventories[index] = stamp(inventory, version, commit)
    notices = output.parent.parent / "licenses"
    if npm:
        notices.mkdir(parents=True, exist_ok=True)
        inventories.extend(npm_inventory(directory, notices) for directory in npm)
    result = package_inventory(name, version, ecosystem, inventories)
    root = result["metadata"]["component"]
    if classifier:
        old_ref = root["bom-ref"]
        root["purl"] += f"?classifier={quote(classifier, safe='')}"
        root["bom-ref"] = root["purl"]
        next(entry for entry in result["dependencies"] if entry["ref"] == old_ref)[
            "ref"
        ] = root["bom-ref"]
    root["properties"] = [
        {
            "name": "maplibre:source-commit",
            "value": git(REPOSITORY, "rev-parse", "HEAD"),
        }
    ]
    for dependency in [*dependencies, *optional_dependencies]:
        dependency_name, dependency_version = dependency.rsplit("@", 1)
        item = package_inventory(dependency_name, dependency_version, ecosystem, [])[
            "metadata"
        ]["component"]
        item.pop("licenses")
        item.pop("group")
        if not re.fullmatch(
            r"\d+\.\d+\.\d+(?:[-+][0-9A-Za-z.-]+)?", dependency_version
        ):
            item.pop("version")
            item["purl"] = item["purl"].rsplit("@", 1)[0]
            item["bom-ref"] = item["purl"]
            item["properties"] = [
                {"name": "maplibre:version-requirement", "value": dependency_version}
            ]
        if dependency in optional_dependencies:
            item["scope"] = "optional"
        result["components"].append(item)
        next(
            entry for entry in result["dependencies"] if entry["ref"] == root["bom-ref"]
        )["dependsOn"].append(item["bom-ref"])
    write_json(output, result)
    return result


def framework(directory: pathlib.Path, version: str) -> None:
    from native_package_release import FRAMEWORK_INFO_PLIST, framework_plist_bytes

    for path in directory.rglob("*"):
        if not path.is_file() or path.is_symlink():
            continue
        if path.name == SBOM_NAME:
            inventory = json.loads(path.read_text())
            commit = next(
                p["value"]
                for p in inventory["metadata"]["component"]["properties"]
                if p["name"] == "maplibre:source-commit"
            )
            write_json(path, stamp(inventory, version, commit))
        elif FRAMEWORK_INFO_PLIST.search(path.as_posix()):
            path.write_bytes(framework_plist_bytes(path.read_bytes(), version))


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    package_parser = commands.add_parser("package")
    for argument in ("name", "version", "ecosystem"):
        package_parser.add_argument(f"--{argument}", required=True)
    package_parser.add_argument(
        "--input", action="append", type=pathlib.Path, default=[]
    )
    package_parser.add_argument("--npm", action="append", type=pathlib.Path, default=[])
    package_parser.add_argument("--dependency", action="append", default=[])
    package_parser.add_argument("--optional-dependency", action="append", default=[])
    package_parser.add_argument("--classifier", default="")
    package_parser.add_argument("--output", required=True, type=pathlib.Path)
    framework_parser = commands.add_parser("framework")
    framework_parser.add_argument("directory", type=pathlib.Path)
    framework_parser.add_argument("version")
    args = parser.parse_args()
    if args.command == "package":
        package(
            args.name,
            args.version,
            args.ecosystem,
            args.input,
            args.output,
            args.npm,
            args.dependency,
            args.optional_dependency,
            args.classifier,
        )
    else:
        framework(args.directory, args.version)


if __name__ == "__main__":
    main()
