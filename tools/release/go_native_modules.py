#!/usr/bin/env python3
"""Packages a release's native archives as Go modules, one per preset, and
publishes them through the Go module proxy protocol.

Each docs page at a preset module's path names this repository's release
downloads under `go/` as the module's proxy. A preset's GitHub release, tagged
`go/<module>/@v`, holds the files that the go command fetches from
`<proxy>/<module>/@v/`: `list`, and each version's `.info`, `.mod`, and `.zip`.
"""

from __future__ import annotations

import argparse
import datetime
import io
import json
import pathlib
import subprocess
import sys
import tarfile
import tempfile
import zipfile

from release_version import parse_tag, parse_version

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[2]))
from tools.ci import gaps

ROOT = pathlib.Path(__file__).resolve().parents[2]
PRESETS = pathlib.Path("bindings/go/presets.json")
MODULE_PREFIX = "maplibre.org/maplibre-native-ffi/go-native"
BINDING_MODULE = "maplibre.org/maplibre-native-ffi/bindings/go/maplibre"
GO_VERSION = "1.26"
ARCHIVE_PREFIX = "maplibre-native-c-"
ARCHIVE_SUFFIX = ".tar.gz"
# The archive's libraries that the module carries, as the pkg-config file
# names them.
ARCHIVES = {
    "-lmaplibre-native-c": "libmaplibre-native-c.a",
    "-lmln_ffi_platform": "libmln_ffi_platform.a",
}
DESCRIPTOR = "share/maplibre-native-c/artifact.json"
PKG_CONFIG = "share/pkgconfig/maplibre-native-c.pc"
# A Go build constraint holds for every GOOS that a port implies: android
# implies linux, and ios implies darwin.
GOOS = {
    "android": "android",
    "freebsd": "freebsd",
    "ios": "ios",
    "ios-maccatalyst": "ios",
    "linux-gnu": "linux && !android",
    "linux-musl": "linux && !android",
    "macos": "darwin && !ios",
    "ohos": "linux && !android",
}
GOARCH = {
    "arm": "arm",
    "arm64": "arm64",
    "loongarch64": "loong64",
    "riscv64": "riscv64",
    "x64": "amd64",
    "x86": "386",
}


def module_path(preset: str) -> str:
    return f"{MODULE_PREFIX}/{preset}"


def proxy_tag(preset: str) -> str:
    """The tag of the release that serves the preset's module."""
    return f"go/{module_path(preset)}/@v"


def presets(root: pathlib.Path = ROOT) -> list[str]:
    return json.loads((root / PRESETS).read_text(encoding="utf-8"))


def constraint(preset: str) -> str:
    """The build constraint for the platform that the preset targets."""
    target = gaps.target_from_name(preset)
    return f"{GOOS[target.platform]} && {GOARCH[target.arch]}"


def link_flags(prefix: pathlib.Path) -> list[str]:
    """The flags that link the install at `prefix` statically, with each
    archive's path relative to the package directory."""
    libs = next(
        line.removeprefix("Libs:").split()
        for line in (prefix / PKG_CONFIG).read_text(encoding="utf-8").splitlines()
        if line.startswith("Libs:")
    )
    flags = []
    for flag in libs:
        if flag in ARCHIVES:
            flags.append(f"${{SRCDIR}}/{ARCHIVES[flag]}")
        elif "${libdir}" not in flag:
            flags.append(flag)
    link = json.loads((prefix / DESCRIPTOR).read_text(encoding="utf-8"))["link"]
    for library in link["libraries"]:
        if f"-l{library}" not in flags:
            flags.append(f"-l{library}")
    for framework in link["frameworks"]:
        flags += ["-framework", framework]
    return flags


def binding_headers(root: pathlib.Path) -> dict[str, bytes]:
    """The installed headers that the Go binding compiles against: include/,
    without what bindings read from the source tree alone."""
    include = root / "include"
    return {
        path.relative_to(include).as_posix(): path.read_bytes()
        for path in include.rglob("*.h")
        if path.name != "binding_support.h"
    }


def check_headers(root: pathlib.Path, prefix: pathlib.Path) -> None:
    """Checks that the install at `prefix` has the headers that the Go binding
    compiles against."""
    installed = prefix / "include"
    archive = {
        path.relative_to(installed).as_posix(): path.read_bytes()
        for path in installed.rglob("*.h")
    }
    if archive != binding_headers(root):
        raise SystemExit(f"{installed} differs from the Go binding's headers")


def go_mod(preset: str) -> str:
    return f"module {module_path(preset)}\n\ngo {GO_VERSION}\n"


def module_files(prefix: pathlib.Path, preset: str, version: str) -> dict[str, bytes]:
    """The files of the preset's module, by path in the module. The binding
    reads mln_go_preset_version, the release that the module belongs to, and
    checks it against its own module's version."""
    flags = " ".join(link_flags(prefix))
    doc = f"""// Package native links the MapLibre Native C library for the {preset}
// preset into a program that imports {BINDING_MODULE}.
// Import it for its side effect:
//
//	import _ "{module_path(preset)}"
//
// It links nothing on another platform, or with the mln_pkgconfig build tag.
package native
"""
    native = f"""//go:build {constraint(preset)} && !mln_pkgconfig

package native

/*
#cgo LDFLAGS: {flags}
const char mln_go_preset_version[] = "{version}";
*/
import "C"
"""
    files = {
        "go.mod": go_mod(preset).encode(),
        "doc.go": doc.encode(),
        "native.go": native.encode(),
        "LICENSE": (prefix / "share/maplibre-native-c/LICENSE").read_bytes(),
    }
    for path in sorted((prefix / "share/maplibre-native-c/licenses").iterdir()):
        files[f"licenses/{path.name}"] = path.read_bytes()
    for archive in ARCHIVES.values():
        path = prefix / "lib" / archive
        if path.is_file():
            files[archive] = path.read_bytes()
    return files


def module_zip(module: str, version: str, files: dict[str, bytes]) -> bytes:
    """A module zip as the go command expects it, with fixed timestamps."""
    buffer = io.BytesIO()
    with zipfile.ZipFile(buffer, "w", zipfile.ZIP_DEFLATED) as archive:
        for path in sorted(files):
            info = zipfile.ZipInfo(f"{module}@{version}/{path}", (1980, 1, 1, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o644 << 16
            archive.writestr(info, files[path])
    return buffer.getvalue()


def build(
    archive: pathlib.Path, version: str, time: str, root: pathlib.Path = ROOT
) -> tuple[str, dict[str, bytes]]:
    """The preset and its module's proxy files for one native archive."""
    preset = archive.name.removeprefix(ARCHIVE_PREFIX).removesuffix(ARCHIVE_SUFFIX)
    parse_version(version.removeprefix("v"))
    with tempfile.TemporaryDirectory() as temporary:
        with tarfile.open(archive) as source:
            source.extractall(temporary, filter="data")
        prefix = pathlib.Path(temporary, f"{ARCHIVE_PREFIX}{preset}")
        check_headers(root, prefix)
        files = module_files(prefix, preset, version)
    info = json.dumps({"Version": version, "Time": time}) + "\n"
    return preset, {
        f"{version}.info": info.encode(),
        f"{version}.mod": files["go.mod"],
        f"{version}.zip": module_zip(module_path(preset), version, files),
    }


def write_proxy(
    out: pathlib.Path, preset: str, version: str, files: dict[str, bytes]
) -> None:
    """Writes the files in a GOPROXY=file:// layout."""
    directory = out / module_path(preset) / "@v"
    directory.mkdir(parents=True, exist_ok=True)
    for name, content in files.items():
        (directory / name).write_bytes(content)
    (directory / "list").write_text(f"{version}\n")


def versions_list(existing: str, version: str) -> str:
    versions = {line for line in existing.split() if line} | {version}
    return "".join(
        f"{v}\n"
        for v in sorted(versions, key=lambda v: parse_version(v.removeprefix("v")))
    )


def gh(*arguments: str, check: bool = True, **kwargs) -> subprocess.CompletedProcess:
    return subprocess.run(["gh", *arguments], check=check, text=True, **kwargs)


def publish_preset(
    preset: str, version: str, sha: str, files: dict[str, bytes]
) -> None:
    """Uploads a version to the preset's release, creating the release first
    if needed. A rerun replaces files with the same contents."""
    tag = proxy_tag(preset)
    if gh("release", "view", tag, check=False, capture_output=True).returncode:
        gh(
            "release",
            "create",
            tag,
            "--target",
            sha,
            "--latest=false",
            "--title",
            f"Go module {module_path(preset)}",
            "--notes",
            f"Serves the Go module `{module_path(preset)}` to the go command. "
            "Its versions follow the MapLibre Native FFI releases.",
        )
    listed = gh(
        "release",
        "download",
        tag,
        "--pattern",
        "list",
        "--output",
        "-",
        check=False,
        capture_output=True,
    )
    with tempfile.TemporaryDirectory() as temporary:
        paths = []
        for name, content in {
            **files,
            "list": versions_list(
                listed.stdout if listed.returncode == 0 else "", version
            ).encode(),
        }.items():
            path = pathlib.Path(temporary, name)
            path.write_bytes(content)
            paths.append(str(path))
        # The list goes last, so the go command finds a version only once its
        # files are in place.
        gh("release", "upload", tag, "--clobber", *paths[:-1])
        gh("release", "upload", tag, "--clobber", paths[-1])


def commit_time(sha: str) -> str:
    """The commit's time in UTC, as the proxy protocol's .info files give it."""
    seconds = subprocess.run(
        ["git", "show", "-s", "--format=%ct", sha],
        check=True,
        text=True,
        capture_output=True,
    ).stdout.strip()
    time = datetime.datetime.fromtimestamp(int(seconds), datetime.UTC)
    return time.strftime("%Y-%m-%dT%H:%M:%SZ")


def main(arguments: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    build_parser = commands.add_parser(
        "build", help="Write one archive's module in a GOPROXY=file:// layout"
    )
    build_parser.add_argument("archive", type=pathlib.Path)
    build_parser.add_argument("version")
    build_parser.add_argument("out", type=pathlib.Path)
    publish_parser = commands.add_parser(
        "publish", help="Publish every preset's module for a release"
    )
    publish_parser.add_argument("tag")
    publish_parser.add_argument("sha")
    publish_parser.add_argument("assets", type=pathlib.Path)
    args = parser.parse_args(arguments)

    if args.command == "build":
        preset, files = build(args.archive, args.version, "2026-01-01T00:00:00Z")
        write_proxy(args.out, preset, args.version, files)
        return 0

    version = "v" + parse_tag(args.tag)[1]
    time = commit_time(args.sha)
    for preset in presets():
        archive = args.assets / f"{ARCHIVE_PREFIX}{preset}{ARCHIVE_SUFFIX}"
        _, files = build(archive, version, time)
        publish_preset(preset, version, args.sha, files)
        print(f"Published {module_path(preset)}@{version}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
