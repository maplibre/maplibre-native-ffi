"""Writes a vcpkg Git registry over the native release archives.

A consumer selects exactly one renderer feature. The prebuilt prefix exports
both shared and static CMake targets, independent of the triplet's preferred
linkage. Published binaries are Release builds, including for Debug consumers.
"""

from __future__ import annotations

import hashlib
import json
import pathlib
import subprocess
import tempfile
from typing import TYPE_CHECKING

from native_package_release import ARCHIVE_PREFIX, ARCHIVE_SUFFIX
from release_version import parse_tag

if TYPE_CHECKING:
    from distribution import Release

PORT = "maplibre-native-c"
# vcpkg's built-in Linux triplets target glibc. The other archive platforms
# have no matching built-in triplet, or use a different package layout.
PLATFORMS = {
    "linux-gnu-x64": "linux-x64",
    "linux-gnu-arm64": "linux-arm64",
    "macos-arm64": "osx-arm64",
    "windows-x64": "windows-x64",
    "windows-arm64": "windows-arm64",
    "freebsd-x64": "freebsd-x64",
    "android-arm": "android-arm",
    "android-arm64": "android-arm64",
    "android-x64": "android-x64",
}
SYSTEMS = {
    "linux": "Linux",
    "osx": "Darwin",
    "windows": "",
    "freebsd": "FreeBSD",
    "android": "Android",
}
FEATURES = {
    "opengl": "OpenGL (EGL, or WGL on Windows)",
    "metal": "Metal (macOS)",
    "vulkan": "Vulkan",
}
PORTFILE = """\
# These release prefixes export both shared and static targets. They are
# already built and include Release binaries for every consumer configuration.
set(VCPKG_POLICY_SKIP_ALL_POST_BUILD_CHECKS enabled)

set(backend "")
foreach(feature IN ITEMS opengl metal vulkan)
    if(feature IN_LIST FEATURES)
        if(NOT backend STREQUAL "")
            message(FATAL_ERROR "Select exactly one maplibre-native-c renderer feature")
        endif()
        set(backend "${feature}")
    endif()
endforeach()
if(backend STREQUAL "")
    message(FATAL_ERROR "Select a maplibre-native-c renderer feature: opengl, metal, or vulkan")
endif()

include("${CMAKE_CURRENT_LIST_DIR}/archives.cmake")
if(NOT DEFINED archive)
    message(FATAL_ERROR "No maplibre-native-c archive for ${TARGET_TRIPLET} with ${backend}")
endif()
vcpkg_download_distfile(download
    URLS "${url}"
    FILENAME "${archive}"
    SHA512 "${sha512}"
)
vcpkg_extract_source_archive(source ARCHIVE "${download}")
file(INSTALL "${source}/" DESTINATION "${CURRENT_PACKAGES_DIR}")
file(RENAME "${CURRENT_PACKAGES_DIR}/share/maplibre-native-c/LICENSE"
            "${CURRENT_PACKAGES_DIR}/share/maplibre-native-c/copyright")
file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage"
     DESTINATION "${CURRENT_PACKAGES_DIR}/share/maplibre-native-c")
"""
USAGE = """\
Select exactly one feature: opengl, metal, or vulkan. Available archives cover
Linux (glibc), macOS arm64, Windows, FreeBSD x64, and Android. An unsupported
platform, architecture, or renderer combination fails during installation.
Both shared and static targets use the release's Release binaries:

    find_package(maplibre-native-c CONFIG REQUIRED)
    target_link_libraries(app PRIVATE maplibre-native-c::maplibre-native-c)

Use maplibre-native-c::maplibre-native-c-static for static linkage.
"""


def write_json(path: pathlib.Path, value: object) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8")


def port_tree(tree: pathlib.Path) -> str:
    """Hash the port with Git's clean index, without touching the caller's index."""
    with tempfile.TemporaryDirectory() as temporary:
        subprocess.run(
            ["git", "init", "--quiet", "--object-format=sha1", temporary], check=True
        )
        command = ["git", "-C", temporary, f"--work-tree={tree.resolve()}"]
        subprocess.run([*command, "add", "--force", f"ports/{PORT}"], check=True)
        root = subprocess.check_output([*command, "write-tree"], text=True).strip()
        return subprocess.check_output(
            [*command, "rev-parse", f"{root}:ports/{PORT}"], text=True
        ).strip()


def write(tree: pathlib.Path, release: Release, repository: pathlib.Path) -> None:
    archives = sorted(release.assets.glob(f"{ARCHIVE_PREFIX}*{ARCHIVE_SUFFIX}"))
    if release.local and not archives:
        return
    version = "0.202601.0" if release.local else parse_tag(release.tag)[1]
    port = tree / "ports" / PORT
    port.mkdir(parents=True)
    selections = []
    for archive in archives:
        preset = archive.name.removeprefix(ARCHIVE_PREFIX).removesuffix(ARCHIVE_SUFFIX)
        platform, _, renderer = preset.rpartition("-")
        if platform not in PLATFORMS:
            continue
        backend = "opengl" if renderer in {"egl", "wgl"} else renderer
        if backend not in FEATURES:
            continue
        with archive.open("rb") as source:
            sha256 = hashlib.file_digest(source, "sha256").hexdigest()
            source.seek(0)
            sha512 = hashlib.file_digest(source, "sha512").hexdigest()
        if not release.local and sha256 != release.sha256(archive.name):
            raise SystemExit(f"{archive.name} does not match SHA256SUMS")
        system, _, architecture = PLATFORMS[platform].rpartition("-")
        url = archive.resolve().as_uri() if release.local else release.url(archive.name)
        selections.append(
            f'if("${{VCPKG_CMAKE_SYSTEM_NAME}}" STREQUAL "{SYSTEMS[system]}"\n'
            f'   AND VCPKG_TARGET_ARCHITECTURE STREQUAL "{architecture}"\n'
            f'   AND backend STREQUAL "{backend}")\n'
            f'    set(archive "{archive.name}")\n'
            f'    set(url "{url}")\n'
            f'    set(sha512 "{sha512}")\n'
            "endif()\n"
        )
    if not selections:
        raise SystemExit("no native archives for the vcpkg registry")
    write_json(
        port / "vcpkg.json",
        {
            "name": PORT,
            "version": version,
            "description": "MapLibre Native C API (prebuilt release)",
            "homepage": "https://maplibre.org/maplibre-native-ffi/",
            "license": "BSD-2-Clause",
            "supports": "(linux & (x64 | arm64)) | (osx & arm64) | (windows & !mingw & !uwp & (x64 | arm64)) | (freebsd & x64) | (android & (arm | arm64 | x64))",
            "features": {
                name: {"description": description}
                for name, description in FEATURES.items()
            },
        },
    )
    (port / "portfile.cmake").write_text(PORTFILE, encoding="utf-8")
    (port / "archives.cmake").write_text("\n".join(selections), encoding="utf-8")
    (port / "usage").write_text(USAGE, encoding="utf-8")
    database = tree / "versions" / "m-" / f"{PORT}.json"
    versions = (
        json.loads(database.read_text("utf-8"))["versions"] if database.exists() else []
    )
    entry = {"git-tree": port_tree(tree), "version": version, "port-version": 0}
    existing = next((item for item in versions if item["version"] == version), None)
    if existing is not None and existing != entry:
        raise SystemExit(f"vcpkg version {version} already names another port tree")
    if existing is None:
        versions.insert(0, entry)
    write_json(database, {"versions": versions})
    write_json(
        tree / "versions/baseline.json",
        {"default": {PORT: {"baseline": version, "port-version": 0}}},
    )
