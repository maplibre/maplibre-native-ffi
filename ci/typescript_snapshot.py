#!/usr/bin/env python3
"""Prepare the TypeScript runtime payloads and their static npm registry."""

from __future__ import annotations

import argparse
import base64
import hashlib
import json
import pathlib
import shutil
import tarfile
import urllib.error
import urllib.parse
import urllib.request


SCOPE = "@maplibre"
# The facade discovers a payload by importing the package named for the host it
# is running on, so a registry that serves one has to serve them under exactly
# those names.
PREFIX = "native-ffi-runtime-"


def _manifest(tarball: pathlib.Path) -> dict:
    with tarfile.open(tarball) as archive:
        member = archive.extractfile("package/package.json")
        if member is None:
            raise SystemExit(f"{tarball}: has no package.json")
        return json.loads(member.read())


def relabel(tarball: pathlib.Path, output: pathlib.Path, version: str) -> None:
    """Rewrites a packed payload to the snapshot version and repacks it.

    A payload is packed by the job that built it, which knows the target but
    not the version the snapshot will carry. Rewriting here keeps every payload
    in one snapshot on one version without rebuilding any of them.
    """
    output.mkdir(parents=True, exist_ok=True)
    manifest = _manifest(tarball)
    name = manifest["name"]
    if not name.startswith(f"{SCOPE}/{PREFIX}"):
        raise SystemExit(f"{tarball}: {name} is not a MapLibre Native payload")

    staging = output / "unpacked"
    if staging.exists():
        shutil.rmtree(staging)
    with tarfile.open(tarball) as archive:
        archive.extractall(staging, filter="data")

    package = staging / "package"
    manifest["version"] = version
    (package / "package.json").write_text(json.dumps(manifest, indent=2) + "\n")

    stem = f"{name.removeprefix(f'{SCOPE}/')}-{version}"
    destination = output / f"{stem}.tgz"
    # npm reads a tarball whose entries all sit under `package/`, and orders
    # them the way it packed them; neither the mtime nor the order matters to
    # it, so the simple repack is enough.
    with tarfile.open(destination, "w:gz") as archive:
        archive.add(package, arcname="package")
    shutil.rmtree(staging)
    print(f"relabelled {name} to {version} as {destination.name}")


def registry(payloads: pathlib.Path, output: pathlib.Path, base_url: str) -> None:
    """Writes the packuments an npm client reads to resolve these packages.

    npm asks a registry for one document per package and finds the tarball
    through the URL that document carries, so the documents are what make a
    directory of files installable. `base_url` is where they will be served
    from, because a packument names its tarball absolutely.
    """
    if output.exists():
        shutil.rmtree(output)
    output.mkdir(parents=True)
    base = base_url.rstrip("/")

    found = sorted(payloads.rglob("*.tgz"))
    if not found:
        raise SystemExit(f"no payload tarballs under {payloads}")

    names = []
    for tarball in found:
        manifest = _manifest(tarball)
        name = manifest["name"]
        version = manifest["version"]
        unscoped = name.removeprefix(f"{SCOPE}/")

        package_dir = output / SCOPE
        package_dir.mkdir(parents=True, exist_ok=True)
        artifact = package_dir / f"{unscoped}-{version}.tgz"
        shutil.copy2(tarball, artifact)
        digest = hashlib.sha512(artifact.read_bytes()).digest()
        integrity = "sha512-" + base64.b64encode(digest).decode()

        packument = {
            "name": name,
            "dist-tags": {"latest": version},
            "versions": {
                version: {
                    **manifest,
                    "dist": {
                        "tarball": f"{base}/{SCOPE}/{artifact.name}",
                        "integrity": integrity,
                        "shasum": hashlib.sha1(artifact.read_bytes()).hexdigest(),
                    },
                }
            },
        }
        # npm requests a scoped package as `@scope%2fname`. A static host
        # decodes that to a path, so the document is written where the decoded
        # path lands, without an extension: the client asks for the name, not a
        # file.
        (package_dir / unscoped).write_text(json.dumps(packument) + "\n")
        names.append(name)

    if len(set(names)) != len(names):
        raise SystemExit(f"two payloads claim the same name: {sorted(names)}")
    (output / "index.json").write_text(json.dumps(sorted(names), indent=2) + "\n")
    print(f"registry serves {len(names)} payloads from {base}")


def mirror(url: str, output: pathlib.Path) -> None:
    """Copies the published registry, so a deploy of something else keeps it.

    A Pages deploy replaces the whole site, so a run that publishes only the
    docs would erase payloads a separately gated component published. This
    carries the live ones forward.
    """
    base = url.rstrip("/")
    output.mkdir(parents=True, exist_ok=True)
    try:
        listing = urllib.request.urlopen(f"{base}/index.json").read()
    except urllib.error.HTTPError as error:
        if error.code != 404:
            raise
        # Nothing published yet, which is not a failure the first time.
        (output / "index.json").write_text("[]\n")
        return
    (output / "index.json").write_bytes(listing)

    package_dir = output / SCOPE
    package_dir.mkdir(parents=True, exist_ok=True)
    for name in json.loads(listing):
        unscoped = name.removeprefix(f"{SCOPE}/")
        document = urllib.request.urlopen(f"{base}/{SCOPE}/{unscoped}").read()
        (package_dir / unscoped).write_bytes(document)
        for release in json.loads(document)["versions"].values():
            tarball = release["dist"]["tarball"]
            filename = urllib.parse.urlsplit(tarball).path.rsplit("/", 1)[-1]
            (package_dir / filename).write_bytes(urllib.request.urlopen(tarball).read())
    print(f"mirrored {len(json.loads(listing))} payloads from {base}")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)

    relabel_command = commands.add_parser("relabel")
    relabel_command.add_argument("tarball", type=pathlib.Path)
    relabel_command.add_argument("output", type=pathlib.Path)
    relabel_command.add_argument("version")

    registry_command = commands.add_parser("registry")
    registry_command.add_argument("payloads", type=pathlib.Path)
    registry_command.add_argument("output", type=pathlib.Path)
    registry_command.add_argument("base_url")

    mirror_command = commands.add_parser("mirror")
    mirror_command.add_argument("url")
    mirror_command.add_argument("output", type=pathlib.Path)

    arguments = parser.parse_args()
    if arguments.command == "relabel":
        relabel(arguments.tarball, arguments.output, arguments.version)
    elif arguments.command == "registry":
        registry(arguments.payloads, arguments.output, arguments.base_url)
    else:
        mirror(arguments.url, arguments.output)


if __name__ == "__main__":
    main()
