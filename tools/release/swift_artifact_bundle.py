"""Writes the C library's SwiftPM artifact bundles from the native archives.

Off Apple platforms, SwiftPM links a C library from a static library artifact
bundle (SE-0482). Each bundle holds one backend's library, in one variant per
platform. Linux and Windows have both backends, so a package selects a bundle
by backend rather than by platform.

A variant holds one library, so it merges the C archive with the Rust platform
archive, which a GNU linker would otherwise need after it on the command line.
The headers sit under MaplibreNativeC/, as in the XCFramework's framework, and
so does its module map, after a header that defines MLN_STATIC, which the
declarations on Windows need to link statically.
"""

from __future__ import annotations

import json
import pathlib
import shutil
import tarfile
import tempfile
import zipfile

ARCHIVE_SUFFIX = ".tar.gz"
# The target triples of each platform with a Swift SDK. SwiftPM matches the
# Android triples at any API level.
TRIPLES = {
    "linux-gnu-x64": ["x86_64-unknown-linux-gnu"],
    "linux-gnu-arm64": ["aarch64-unknown-linux-gnu"],
    "windows-x64": ["x86_64-unknown-windows-msvc"],
    "windows-arm64": ["aarch64-unknown-windows-msvc"],
    "android-arm": ["armv7-unknown-linux-androideabi"],
    "android-arm64": ["aarch64-unknown-linux-android"],
    "android-x64": ["x86_64-unknown-linux-android"],
}
# The artifact, and the binary target that names it, of each backend.
ARTIFACTS = {"opengl": "MaplibreNativeCOpenGL", "vulkan": "MaplibreNativeCVulkan"}
STATIC_HEADER = "maplibre_native_c_static.h"
MODULE_MAP = f"""\
// {STATIC_HEADER} comes first to define MLN_STATIC for the headers after it.
module MaplibreNativeC [system] {{
  header "{STATIC_HEADER}"
  header "MaplibreNativeC/maplibre_native_c.h"
  export *
}}
"""
STATIC_HEADER_TEXT = """\
// The artifact bundle links the library statically.
#ifndef MLN_STATIC
#define MLN_STATIC
#endif
"""


def bundle_name(backend: str) -> str:
    return f"{ARTIFACTS[backend]}.artifactbundle.zip"


def descriptor(root: pathlib.Path) -> dict:
    return json.loads(
        (root / "share/maplibre-native-c/artifact.json").read_text("utf-8")
    )


def copy_archive(root: pathlib.Path, output: pathlib.Path) -> None:
    """Copies the prefix's static archive to `output`."""
    lib = root / "lib"
    if output.suffix == ".lib":
        source = lib / "maplibre-native-c-static.lib"
    else:
        source = lib / "libmaplibre-native-c.a"
    shutil.copyfile(source, output)


def headers(root: pathlib.Path) -> dict[str, bytes]:
    """The prefix's headers, whose line endings follow the build's checkout."""
    return {
        path.relative_to(root / "include").as_posix(): path.read_bytes().replace(
            b"\r\n", b"\n"
        )
        for path in sorted((root / "include").rglob("*"))
        if path.is_file()
    }


def write_zip(directory: pathlib.Path, output: pathlib.Path) -> None:
    """Zips `directory` under its own name, with fixed timestamps."""
    with zipfile.ZipFile(output, "w", zipfile.ZIP_DEFLATED) as archive:
        for path in sorted(directory.rglob("*")):
            if path.is_file():
                name = path.relative_to(directory.parent).as_posix()
                member = zipfile.ZipInfo(name, (1980, 1, 1, 0, 0, 0))
                member.external_attr = 0o644 << 16
                member.compress_type = zipfile.ZIP_DEFLATED
                with path.open("rb") as source, archive.open(member, "w") as sink:
                    shutil.copyfileobj(source, sink, 1024 * 1024)


def write_bundles(
    output_dir: pathlib.Path, archives: list[pathlib.Path], version: str
) -> dict[str, tuple[pathlib.Path, list[dict]]]:
    """Writes a bundle for each backend of the archives for Swift's platforms.

    Returns each backend's bundle and the inventories of its variants. An
    archive for another platform or backend joins no bundle.
    """
    bundles: dict[str, tuple[pathlib.Path, list[dict]]] = {}
    with tempfile.TemporaryDirectory() as temporary:
        work = pathlib.Path(temporary)
        roots: dict[str, list[pathlib.Path]] = {}
        for archive in sorted(archives):
            name = archive.name.removesuffix(ARCHIVE_SUFFIX)
            with tarfile.open(archive, "r:gz") as source:
                prefix = f"{name}/share/maplibre-native-c/artifact.json"
                found = json.load(source.extractfile(prefix))
                if (
                    found["platform"]["id"] not in TRIPLES
                    or found["backend"] not in ARTIFACTS
                ):
                    continue
                source.extractall(work / "archives", filter="data")
            roots.setdefault(found["backend"], []).append(work / "archives" / name)

        for backend, prefixes in sorted(roots.items()):
            artifact = ARTIFACTS[backend]
            bundle = work / backend / f"{artifact}.artifactbundle"
            include = headers(prefixes[0])
            variants = []
            inventories = []
            for root in prefixes:
                if headers(root) != include:
                    raise SystemExit(
                        f"{root.name} has other headers than {prefixes[0].name}"
                    )
                platform = descriptor(root)["platform"]["id"]
                library = (
                    "maplibre-native-c.lib"
                    if platform.startswith("windows-")
                    else "libmaplibre-native-c.a"
                )
                (bundle / platform).mkdir(parents=True)
                copy_archive(root, bundle / platform / library)
                shutil.copytree(
                    root / "share/maplibre-native-c", bundle / platform / "share"
                )
                sbom = bundle / platform / "share/sbom.cdx.json"
                if sbom.is_file():
                    inventories.append(json.loads(sbom.read_text("utf-8")))
                variants.append(
                    {
                        "path": f"{platform}/{library}",
                        "supportedTriples": TRIPLES[platform],
                        "staticLibraryMetadata": {
                            "headerPaths": ["include"],
                            "moduleMapPath": "include/module.modulemap",
                        },
                    }
                )
            shutil.copyfile(
                prefixes[0] / "share/maplibre-native-c/LICENSE", bundle / "LICENSE"
            )
            shutil.copytree(prefixes[0] / "include", bundle / "include/MaplibreNativeC")
            (bundle / "include" / STATIC_HEADER).write_text(
                STATIC_HEADER_TEXT, encoding="utf-8"
            )
            (bundle / "include/module.modulemap").write_text(
                MODULE_MAP, encoding="utf-8"
            )
            info = {
                "schemaVersion": "1.0",
                "artifacts": {
                    artifact: {
                        "type": "staticLibrary",
                        "version": version,
                        "variants": variants,
                    }
                },
            }
            (bundle / "info.json").write_text(
                json.dumps(info, indent=2) + "\n", encoding="utf-8"
            )
            output = output_dir / bundle_name(backend)
            write_zip(bundle, output)
            bundles[backend] = (output, inventories)
    return bundles
