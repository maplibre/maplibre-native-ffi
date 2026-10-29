"""Build CycloneDX inventories and their accompanying license notices."""

from __future__ import annotations

import argparse
import hashlib
import json
import pathlib
import re
import shutil
import subprocess
from urllib.parse import quote

REPOSITORY = pathlib.Path(__file__).resolve().parents[2]
SBOM_NAME = "sbom.cdx.json"


def git(directory: pathlib.Path, *arguments: str) -> str:
    return subprocess.check_output(
        ["git", "-C", str(directory), *arguments], text=True
    ).strip()


def component(name: str, version: str, license: str, url: str = "") -> dict:
    result = {
        "type": "library",
        "bom-ref": f"{name}@{version}",
        "name": name,
        "version": version,
        "purl": f"pkg:generic/{quote(name, safe='')}@{quote(version, safe='')}",
        "licenses": [{"expression": license}],
    }
    if url:
        result["externalReferences"] = [{"type": "vcs", "url": url}]
    return result


def document(
    root: dict, components: list[dict], dependencies: dict[str, set[str]]
) -> dict:
    return {
        "$schema": "http://cyclonedx.org/schema/bom-1.6.schema.json",
        "bomFormat": "CycloneDX",
        "specVersion": "1.6",
        "version": 1,
        "metadata": {"component": root},
        "components": sorted(components, key=lambda item: item["bom-ref"]),
        "dependencies": [
            {"ref": ref, "dependsOn": sorted(children)}
            for ref, children in sorted(dependencies.items())
        ],
    }


def write_json(path: pathlib.Path, value: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(
        json.dumps(value, indent=2, ensure_ascii=False) + "\n", encoding="utf-8"
    )


def source_version(source: pathlib.Path) -> tuple[str, list[dict]]:
    commit = git(source, "rev-parse", "HEAD")
    checkout = pathlib.Path(git(source, "rev-parse", "--show-toplevel"))
    relative = source.resolve().relative_to(checkout.resolve()).as_posix()
    properties = [{"name": "maplibre:source-commit", "value": commit}]
    if relative != ".":
        properties.append({"name": "maplibre:source-path", "value": relative})
    # These copies are embedded in the engine repository rather than pinned as
    # submodules. Retain their upstream versions as well as the engine revision.
    for header, macro in (
        ("include/unicode/uvernum.h", "U_ICU_VERSION"),
        ("include/sqlite3.h", "SQLITE_VERSION"),
    ):
        path = source / header
        if path.exists():
            match = re.search(
                rf'^#define {macro}\s+"([^"]+)"', path.read_text(), re.MULTILINE
            )
            if match:
                return match[1], properties
    return commit, properties


def cargo_components(
    about: dict, target: str
) -> tuple[list[dict], dict[str, set[str]], str]:
    packages = {entry["package"]["id"]: entry for entry in about["crates"]}
    manifest = REPOSITORY / "src/platform/rust/Cargo.toml"
    metadata = json.loads(
        subprocess.check_output(
            [
                "cargo",
                "metadata",
                "--format-version",
                "1",
                "--locked",
                "--offline",
                "--manifest-path",
                str(manifest),
                "--filter-platform",
                target,
            ],
            cwd=REPOSITORY,
        )
    )
    refs = {}
    components = []
    root = ""
    for package_id, entry in packages.items():
        package = entry["package"]
        item = component(
            package["name"],
            package["version"],
            entry["license"],
            package.get("repository") or "",
        )
        if package["source"] and package["source"].startswith("registry+"):
            item["purl"] = (
                f"pkg:cargo/{quote(package['name'])}@{quote(package['version'])}"
            )
            item["bom-ref"] = item["purl"]
        else:
            item["group"] = "org.maplibre.nativeffi"
            if package["name"] == "maplibre-native-ffi-platform":
                root = item["bom-ref"]
        refs[package_id] = item["bom-ref"]
        components.append(item)
    dependencies = {}
    for node in metadata["resolve"]["nodes"]:
        if node["id"] not in refs:
            continue
        dependencies[refs[node["id"]]] = {
            refs[dep["pkg"]]
            for dep in node["deps"]
            if dep["pkg"] in refs
            and any(kind["kind"] is None for kind in dep["dep_kinds"])
        }
    if not root:
        raise ValueError("cargo-about inventory has no platform crate")
    return components, dependencies, root


def cargo_notices(about: dict) -> str:
    sections = ["# Rust dependency notices\n"]
    for license in about["licenses"]:
        users = sorted(
            {
                f"{user['crate']['name']} {user['crate']['version']}"
                for user in license["used_by"]
            }
        )
        sections.append(
            f"## {license['name']} ({license['id']})\n\n"
            f"Used by: {', '.join(users)}\n\n```\n{license['text']}\n```\n"
        )
    return "\n".join(sections)


def native(
    records: list[dict],
    artifact: dict,
    output: pathlib.Path,
    about: dict | None = None,
    rust_target: str = "",
) -> dict:
    root = component(
        "maplibre-native-c",
        artifact["version"],
        "BSD-2-Clause",
        "https://github.com/maplibre/maplibre-native-ffi",
    )
    root["bom-ref"] = f"native:{artifact['platform']['id']}:{artifact['backend']}"
    root["group"] = "org.maplibre.nativeffi"
    root["properties"] = [
        {"name": "maplibre:platform", "value": artifact["platform"]["id"]},
        {"name": "maplibre:backend", "value": artifact["backend"]},
        {"name": "maplibre:source-commit", "value": artifact["commit"]},
    ]
    components = []
    refs = {}
    notices = {"maplibre-native-ffi.txt": REPOSITORY / "LICENSE"}
    for record in records:
        version = record["version"]
        properties = []
        if record["source"]:
            source_revision, properties = source_version(pathlib.Path(record["source"]))
            version = version or source_revision
        if not version:
            raise ValueError(f"{record['name']} has no version or source revision")
        item = component(record["name"], version, record["license"], record["url"])
        if properties:
            item["properties"] = properties
        if record["name"] == "maplibre-native":
            patches = sorted((REPOSITORY / "patches/maplibre-native").glob("*.patch"))
            item.setdefault("properties", []).extend(
                {
                    "name": f"maplibre:patch:{patch.name}",
                    "value": hashlib.sha256(patch.read_bytes()).hexdigest(),
                }
                for patch in patches
            )
        refs[record["name"]] = item["bom-ref"]
        components.append(item)
        for name, path in record["notices"].items():
            path = pathlib.Path(path)
            if name in notices and notices[name] != path:
                raise ValueError(f"conflicting notice: {name}")
            notices[name] = path
    dependencies = {root["bom-ref"]: set()}
    for record in records:
        parent = refs[record["parent"]] if record["parent"] else root["bom-ref"]
        dependencies.setdefault(parent, set()).add(refs[record["name"]])
        dependencies.setdefault(refs[record["name"]], set())
    if about is not None:
        crates, graph, platform = cargo_components(about, rust_target)
        components.extend(crates)
        dependencies.update(graph)
        dependencies[root["bom-ref"]].add(platform)
    for name in sorted(
        set(artifact["link"]["libraries"] + artifact["link"]["frameworks"])
    ):
        ref = f"system:{name}"
        components.append(
            {
                "type": "library",
                "name": name,
                "bom-ref": ref,
                "scope": "excluded",
                "properties": [{"name": "maplibre:provided-by", "value": "host"}],
            }
        )
        dependencies[root["bom-ref"]].add(ref)
    # Recreate the generated directory so removing a dependency also removes its
    # notice from the next install and package.
    shutil.rmtree(output, ignore_errors=True)
    (output / "licenses").mkdir(parents=True)
    for name, path in sorted(notices.items()):
        shutil.copyfile(path, output / "licenses" / name)
    if about is not None:
        (output / "licenses/rust.md").write_text(cargo_notices(about), encoding="utf-8")
    result = document(root, components, dependencies)
    write_json(output / SBOM_NAME, result)
    return result


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    native_parser = commands.add_parser("native")
    native_parser.add_argument("--records", required=True, type=pathlib.Path)
    native_parser.add_argument("--artifact", required=True, type=pathlib.Path)
    native_parser.add_argument("--output", required=True, type=pathlib.Path)
    native_parser.add_argument("--cargo-about", type=pathlib.Path)
    native_parser.add_argument("--rust-target", default="")
    args = parser.parse_args()
    native(
        json.loads(args.records.read_text()),
        json.loads(args.artifact.read_text()),
        args.output,
        json.loads(args.cargo_about.read_text()) if args.cargo_about else None,
        args.rust_target,
    )


if __name__ == "__main__":
    main()
