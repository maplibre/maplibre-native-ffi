"""Build backend-specific NuGet packages and a static Sleet feed."""

from __future__ import annotations

import argparse
import json
import pathlib
import shutil
import subprocess
import tarfile
import tempfile
import urllib.error
import urllib.parse
import urllib.request

ROOT = pathlib.Path(__file__).resolve().parent.parent
MANAGED_PROJECT = (
    ROOT / "bindings/dotnet/src/Maplibre.NativeFfi/Maplibre.NativeFfi.csproj"
)
RUNTIME_PROJECT = (
    ROOT
    / "bindings/dotnet/src/Maplibre.NativeFfi.Runtime/Maplibre.NativeFfi.Runtime.csproj"
)

BACKENDS = {
    "OpenGL": {
        "linux-x64-egl": "linux-x64",
        "linux-arm64-egl": "linux-arm64",
        "macos-arm64-egl": "osx-arm64",
        "windows-x64-wgl": "win-x64",
        "windows-arm64-wgl": "win-arm64",
        "android-arm-egl": "android-arm",
        "android-arm64-egl": "android-arm64",
        "android-x64-egl": "android-x64",
    },
    "Vulkan": {
        "linux-x64-vulkan": "linux-x64",
        "linux-arm64-vulkan": "linux-arm64",
        "macos-arm64-vulkan": "osx-arm64",
        "windows-x64-vulkan": "win-x64",
        "windows-arm64-vulkan": "win-arm64",
        "android-arm-vulkan": "android-arm",
        "android-arm64-vulkan": "android-arm64",
        "android-x64-vulkan": "android-x64",
    },
    "Metal": {"macos-arm64-metal": "osx-arm64"},
}


def run(*arguments: str, cwd: pathlib.Path = ROOT) -> None:
    subprocess.run(arguments, cwd=cwd, check=True)


def sleet(*arguments: str) -> None:
    run("dotnet", "tool", "run", "sleet", *arguments, cwd=ROOT / "bindings/dotnet")


def archive(input_dir: pathlib.Path, preset: str) -> pathlib.Path:
    matches = list(input_dir.glob(f"**/maplibre-native-c-{preset}.tar.gz"))
    if len(matches) != 1:
        raise SystemExit(f"expected one archive for {preset}, found {len(matches)}")
    return matches[0]


def dynamic_libraries(directory: pathlib.Path) -> list[pathlib.Path]:
    suffixes = (".so", ".dylib", ".dll")
    return [
        path
        for path in directory.rglob("*")
        if path.is_file() and (path.name.endswith(suffixes) or ".so." in path.name)
    ]


def package(args: argparse.Namespace) -> None:
    args.output.mkdir(parents=True, exist_ok=True)
    run(
        "dotnet",
        "pack",
        str(MANAGED_PROJECT),
        "--configuration",
        "Release",
        "--output",
        str(args.output),
        f"-p:PackageVersion={args.version}",
    )

    with tempfile.TemporaryDirectory() as temporary:
        staging = pathlib.Path(temporary)
        for backend, presets in BACKENDS.items():
            assets = staging / backend
            for preset, rid in presets.items():
                extracted = staging / "extract" / preset
                with tarfile.open(archive(args.input, preset)) as tar:
                    tar.extractall(extracted, filter="data")
                libraries = dynamic_libraries(extracted)
                if not libraries:
                    raise SystemExit(f"{preset} archive contains no dynamic libraries")
                destination = assets / rid / "native"
                destination.mkdir(parents=True, exist_ok=True)
                for library in libraries:
                    shutil.copy2(library, destination / library.name)
                # The libraries bundle third-party code, so the archive's
                # license notices travel with them.
                notices = next(extracted.glob("**/share/*/licenses"), None)
                if notices is None:
                    raise SystemExit(f"{preset} archive carries no licenses")
                shutil.copytree(notices, destination / "licenses", dirs_exist_ok=True)

            run(
                "dotnet",
                "pack",
                str(RUNTIME_PROJECT),
                "--configuration",
                "Release",
                "--output",
                str(args.output),
                f"-p:PackageVersion={args.version}",
                f"-p:RenderBackend={backend}",
                f"-p:NativeAssetsDir={assets}",
            )


def config(path: pathlib.Path, output: pathlib.Path, base_uri: str) -> None:
    path.write_text(
        json.dumps(
            {
                "username": "MapLibre Native FFI",
                "useremail": "noreply@maplibre.org",
                "sources": [
                    {
                        "name": "snapshot",
                        "type": "local",
                        "path": str(output.resolve()),
                        "baseURI": base_uri,
                    }
                ],
            }
        ),
        encoding="utf-8",
    )


def feed(args: argparse.Namespace) -> None:
    if args.output.exists():
        shutil.rmtree(args.output)
    args.output.mkdir(parents=True)
    with tempfile.TemporaryDirectory() as temporary:
        config_path = pathlib.Path(temporary) / "sleet.json"
        config(config_path, args.output, args.base_uri)
        sleet("init", "--config", str(config_path), "--source", "snapshot")
        sleet(
            "push",
            str(args.packages.resolve()),
            "--config",
            str(config_path),
            "--source",
            "snapshot",
        )
        sleet("validate", "--config", str(config_path), "--source", "snapshot")


def urls(value: object) -> list[str]:
    if isinstance(value, str):
        return [value]
    if isinstance(value, list):
        return [url for item in value for url in urls(item)]
    if isinstance(value, dict):
        return [url for item in value.values() for url in urls(item)]
    return []


def mirror(args: argparse.Namespace) -> None:
    source = args.source.rstrip("/") + "/"
    if args.output.exists():
        shutil.rmtree(args.output)
    args.output.mkdir(parents=True)
    pending = [urllib.parse.urljoin(source, "index.json")]
    seen: set[str] = set()
    while pending:
        url = urllib.parse.urldefrag(pending.pop()).url
        if (
            url in seen
            or url == source
            or url.endswith("/")
            or "{" in url
            or not url.startswith(source)
        ):
            continue
        seen.add(url)
        relative = urllib.parse.unquote(url.removeprefix(source))
        destination = args.output / relative
        destination.parent.mkdir(parents=True, exist_ok=True)
        try:
            with urllib.request.urlopen(url) as response:
                content = response.read()
        except urllib.error.HTTPError as error:
            if error.code == 404:
                # The live site has no feed until the first .NET snapshot
                # publishes, and Sleet registrations cite virtual catalog URIs
                # that are never materialized.
                continue
            raise
        destination.write_bytes(content)
        try:
            document = json.loads(content)
        except UnicodeDecodeError, json.JSONDecodeError:
            continue
        pending.extend(
            candidate for candidate in urls(document) if candidate.startswith(source)
        )
        if destination.name == "sleet.packageindex.json":
            for package_id, versions in document["packages"].items():
                lower_id = package_id.lower()
                pending.append(f"{source}flatcontainer/{lower_id}/index.json")
                for version in versions:
                    lower_version = version.lower()
                    root = f"{source}flatcontainer/{lower_id}/{lower_version}/"
                    pending.append(f"{root}{lower_id}.nuspec")
                    # No document on the feed links to the package bytes, so
                    # only an explicit fetch mirrors what a restore needs.
                    pending.append(f"{root}{lower_id}.{lower_version}.nupkg")


def main() -> None:
    parser = argparse.ArgumentParser()
    commands = parser.add_subparsers(dest="command", required=True)
    package_parser = commands.add_parser("package")
    package_parser.add_argument("input", type=pathlib.Path)
    package_parser.add_argument("output", type=pathlib.Path)
    package_parser.add_argument("version")
    feed_parser = commands.add_parser("feed")
    feed_parser.add_argument("packages", type=pathlib.Path)
    feed_parser.add_argument("output", type=pathlib.Path)
    feed_parser.add_argument("base_uri")
    mirror_parser = commands.add_parser("mirror")
    mirror_parser.add_argument("source")
    mirror_parser.add_argument("output", type=pathlib.Path)
    args = parser.parse_args()
    {"package": package, "feed": feed, "mirror": mirror}[args.command](args)


if __name__ == "__main__":
    main()
