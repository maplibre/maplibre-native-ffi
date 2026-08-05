#!/usr/bin/env python3
"""Prepare backend-specific Python wheels and their static package index."""

from __future__ import annotations

import argparse
import base64
import csv
import hashlib
import html
import html.parser
import io
import os
import pathlib
import re
import shutil
import urllib.error
import urllib.parse
import urllib.request
import zipfile


BACKENDS = {"vulkan", "opengl", "metal"}
NAME = "maplibre-native-ffi"
# Wheel filenames and dist-info directories use the escaped distribution name.
WHEEL_NAME = NAME.replace("-", "_")
VERSION = re.compile(r"0\.1\.0\.dev\d{12}\Z")


def _replace_metadata(source: bytes, backend: str, version: str) -> bytes:
    text = source.decode()
    text, names = re.subn(r"(?m)^Name: .+$", f"Name: {NAME}-{backend}", text)
    text, versions = re.subn(r"(?m)^Version: .+$", f"Version: {version}", text)
    if names != 1 or versions != 1:
        raise SystemExit("wheel METADATA has an unexpected Name or Version field")
    return text.encode()


def relabel(
    wheel: pathlib.Path, output: pathlib.Path, backend: str, version: str
) -> None:
    if backend not in BACKENDS:
        raise SystemExit(f"unsupported backend: {backend}")
    if not VERSION.fullmatch(version):
        raise SystemExit(
            f"snapshot version must look like 0.1.0.devYYYYMMDDHHMM: {version}"
        )

    with zipfile.ZipFile(wheel) as archive:
        files = {
            info.filename: archive.read(info)
            for info in archive.infolist()
            if not info.is_dir()
        }
    metadata = [name for name in files if name.endswith(".dist-info/METADATA")]
    if len(metadata) != 1:
        raise SystemExit("wheel must contain exactly one dist-info/METADATA file")
    old_info = metadata[0].split("/", 1)[0]
    new_info = f"{WHEEL_NAME}_{backend}-{version}.dist-info"
    rewritten = {}
    for name, data in files.items():
        new_name = (
            new_info + name[len(old_info) :] if name.startswith(old_info) else name
        )
        if new_name == f"{new_info}/METADATA":
            data = _replace_metadata(data, backend, version)
        if not new_name.endswith(".dist-info/RECORD"):
            rewritten[new_name] = data

    rows = []
    for name, data in sorted(rewritten.items()):
        digest = (
            base64.urlsafe_b64encode(hashlib.sha256(data).digest())
            .rstrip(b"=")
            .decode()
        )
        rows.append((name, f"sha256={digest}", str(len(data))))
    record_name = f"{new_info}/RECORD"
    rows.append((record_name, "", ""))
    record = io.StringIO(newline="")
    csv.writer(record, lineterminator="\n").writerows(rows)
    rewritten[record_name] = record.getvalue().encode()

    tags = wheel.name.split("-", 2)[2]
    destination = output / f"{WHEEL_NAME}_{backend}-{version}-{tags}"
    output.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(destination, "w", compression=zipfile.ZIP_DEFLATED) as archive:
        for name, data in sorted(rewritten.items()):
            archive.writestr(name, data)
    print(destination)


def _metadata(wheel: pathlib.Path) -> tuple[str, str]:
    with zipfile.ZipFile(wheel) as archive:
        names = [
            name for name in archive.namelist() if name.endswith(".dist-info/METADATA")
        ]
        if len(names) != 1:
            raise SystemExit(f"{wheel}: expected one METADATA file")
        metadata = archive.read(names[0]).decode()
    name = re.search(r"(?m)^Name: (.+)$", metadata)
    version = re.search(r"(?m)^Version: (.+)$", metadata)
    if not name or not version:
        raise SystemExit(f"{wheel}: incomplete package metadata")
    return name.group(1), version.group(1)


def index(wheels: pathlib.Path, output: pathlib.Path) -> None:
    if output.exists():
        shutil.rmtree(output)
    output.mkdir(parents=True)
    projects: dict[str, list[pathlib.Path]] = {}
    for wheel in sorted(wheels.rglob("*.whl")):
        name, version = _metadata(wheel)
        if name.removeprefix(f"{NAME}-") not in BACKENDS or not VERSION.fullmatch(
            version
        ):
            raise SystemExit(f"{wheel}: is not a MapLibre Native backend snapshot")
        projects.setdefault(name, []).append(wheel)
    if set(projects) != {f"{NAME}-{backend}" for backend in BACKENDS}:
        raise SystemExit("index requires vulkan, opengl, and metal wheels")

    root_links = []
    for name, found in sorted(projects.items()):
        project_dir = output / name
        project_dir.mkdir()
        links = []
        for wheel in found:
            destination = project_dir / wheel.name
            try:
                os.link(wheel, destination)
            except OSError:
                shutil.copy2(wheel, destination)
            digest = hashlib.sha256(destination.read_bytes()).hexdigest()
            links.append(
                f'<a href="{html.escape(wheel.name)}#sha256={digest}">{html.escape(wheel.name)}</a>'
            )
        (project_dir / "index.html").write_text(
            "<!doctype html>\n" + "<br>\n".join(links) + "\n"
        )
        root_links.append(f'<a href="{name}/">{name}</a>')
    (output / "index.html").write_text(
        "<!doctype html>\n" + "<br>\n".join(root_links) + "\n"
    )


class _Links(html.parser.HTMLParser):
    def __init__(self) -> None:
        super().__init__()
        self.hrefs: list[str] = []

    def handle_starttag(self, tag: str, attrs: list[tuple[str, str | None]]) -> None:
        if tag == "a":
            self.hrefs.extend(value for key, value in attrs if key == "href" and value)


def mirror(url: str, output: pathlib.Path) -> None:
    try:
        root = urllib.request.urlopen(url).read()
    except urllib.error.HTTPError as error:
        if error.code != 404:
            raise
        output.mkdir(parents=True, exist_ok=True)
        (output / "index.html").write_text("<!doctype html>\n")
        return
    output.mkdir(parents=True, exist_ok=True)
    (output / "index.html").write_bytes(root)
    root_parser = _Links()
    root_parser.feed(root.decode())
    for project_href in root_parser.hrefs:
        project_url = urllib.parse.urljoin(url, project_href)
        project = project_href.strip("/")
        project_dir = output / project
        project_dir.mkdir()
        page = urllib.request.urlopen(project_url).read()
        (project_dir / "index.html").write_bytes(page)
        parser = _Links()
        parser.feed(page.decode())
        for wheel_href in parser.hrefs:
            wheel_url = urllib.parse.urljoin(project_url, wheel_href)
            filename = urllib.parse.urlsplit(wheel_url).path.rsplit("/", 1)[-1]
            (project_dir / filename).write_bytes(
                urllib.request.urlopen(wheel_url).read()
            )


def main() -> None:
    parser = argparse.ArgumentParser()
    commands = parser.add_subparsers(dest="command", required=True)
    relabel_parser = commands.add_parser("relabel")
    relabel_parser.add_argument("wheel", type=pathlib.Path)
    relabel_parser.add_argument("output", type=pathlib.Path)
    relabel_parser.add_argument("backend", choices=sorted(BACKENDS))
    relabel_parser.add_argument("version")
    index_parser = commands.add_parser("index")
    index_parser.add_argument("wheels", type=pathlib.Path)
    index_parser.add_argument("output", type=pathlib.Path)
    mirror_parser = commands.add_parser("mirror")
    mirror_parser.add_argument("url")
    mirror_parser.add_argument("output", type=pathlib.Path)
    args = parser.parse_args()
    if args.command == "relabel":
        relabel(args.wheel, args.output, args.backend, args.version)
    elif args.command == "index":
        index(args.wheels, args.output)
    else:
        mirror(args.url, args.output)


if __name__ == "__main__":
    main()
