#!/usr/bin/env python3
import argparse
import json
import os
import shlex
from pathlib import Path
from xml.sax.saxutils import escape as xml_escape


LIBRARY_NAME = "maplibre-native-c"


def unique(values):
    result = []
    seen = set()
    for value in values:
        if not value or value in seen:
            continue
        result.append(value)
        seen.add(value)
    return result


def load_artifact(metadata_path):
    with open(metadata_path, encoding="utf-8") as metadata_file:
        metadata = json.load(metadata_file)

    library_path = metadata["library_path"]
    import_library_path = metadata.get("import_library_path") or library_path
    link_libraries = metadata.get("link_libraries") or [LIBRARY_NAME]
    library_dirs = metadata.get("library_dirs", [])
    rpaths = metadata.get("rpaths", [])

    runtime_library_dirs = unique([str(Path(library_path).parent), *rpaths])
    loader_library_dirs = unique(
        [*runtime_library_dirs, *runtime_dirs_from_library_dirs(library_dirs)]
    )

    return {
        "metadata_path": str(Path(metadata_path).resolve()),
        "render_backend": metadata.get("render_backend", ""),
        "artifact_shape": metadata.get("artifact_shape", ""),
        "library_path": library_path,
        "import_library_path": import_library_path,
        "include_dirs": unique(metadata.get("include_dirs", [])),
        "library_dirs": unique(library_dirs),
        "link_dirs": unique([str(Path(import_library_path).parent), *library_dirs]),
        "runtime_library_dirs": runtime_library_dirs,
        "loader_library_dirs": loader_library_dirs,
        "link_libraries": unique(link_libraries),
        "frameworks": unique(metadata.get("frameworks", [])),
    }


def runtime_dirs_from_library_dirs(library_dirs):
    result = []
    for library_dir in library_dirs:
        path = Path(library_dir)
        if path.is_dir():
            result.append(str(path))
        for sibling_name in ("bin",):
            sibling = path.parent / sibling_name
            if sibling.is_dir():
                result.append(str(sibling))
    return result


def path_list(values):
    return os.pathsep.join(values)


def properties_escape(value):
    return value.replace("\\", "\\\\").replace("\n", "\\n").replace("\r", "\\r")


def pc_escape(value):
    return value.replace("\\", "\\\\").replace(" ", "\\ ").replace("\t", "\\\t")


def linker_flags(artifact):
    flags = []
    for link_dir in artifact["link_dirs"]:
        flags.append(f"-L{link_dir}")
    for link_library in artifact["link_libraries"]:
        flags.append(f"-l{link_library}")
    for framework in artifact["frameworks"]:
        flags.extend(["-framework", framework])
    for runtime_dir in artifact["runtime_library_dirs"]:
        flags.extend(["-Xlinker", "-rpath", "-Xlinker", runtime_dir])
    return flags


def write_pkg_config(artifact, out_dir):
    lines = [
        f"Name: {LIBRARY_NAME}",
        "Description: MapLibre Native C development artifact",
        "Version: 0",
        "Cflags: "
        + " ".join(
            f"-I{pc_escape(include_dir)}" for include_dir in artifact["include_dirs"]
        ),
        "Libs: "
        + " ".join(
            [
                *(f"-L{pc_escape(link_dir)}" for link_dir in artifact["link_dirs"]),
                *(
                    f"-l{pc_escape(link_library)}"
                    for link_library in artifact["link_libraries"]
                ),
                *(
                    f"-Wl,-rpath,{pc_escape(runtime_dir)}"
                    for runtime_dir in artifact["runtime_library_dirs"]
                ),
                *(
                    flag
                    for framework in artifact["frameworks"]
                    for flag in ("-framework", pc_escape(framework))
                ),
            ]
        ),
    ]
    (out_dir / f"{LIBRARY_NAME}.pc").write_text(
        "\n".join(lines) + "\n", encoding="utf-8"
    )


def write_gradle_properties(artifact, out_dir):
    values = {
        "maplibreNativeC.metadataPath": artifact["metadata_path"],
        "maplibreNativeC.renderBackend": artifact["render_backend"],
        "maplibreNativeC.artifactShape": artifact["artifact_shape"],
        "maplibreNativeC.libraryPath": artifact["library_path"],
        "maplibreNativeC.importLibraryPath": artifact["import_library_path"],
        "maplibreNativeC.includeDirs": path_list(artifact["include_dirs"]),
        "maplibreNativeC.libraryDirs": path_list(artifact["library_dirs"]),
        "maplibreNativeC.linkDirs": path_list(artifact["link_dirs"]),
        "maplibreNativeC.runtimeLibraryDirs": path_list(
            artifact["runtime_library_dirs"]
        ),
        "maplibreNativeC.loaderLibraryDirs": path_list(artifact["loader_library_dirs"]),
        "maplibreNativeC.linkLibraries": path_list(artifact["link_libraries"]),
        "maplibreNativeC.frameworks": path_list(artifact["frameworks"]),
        "maplibreNativeC.pkgConfigPath": str(out_dir),
    }
    lines = [f"{key}={properties_escape(value)}" for key, value in values.items()]
    (out_dir / f"{LIBRARY_NAME}.gradle.properties").write_text(
        "\n".join(lines) + "\n",
        encoding="utf-8",
    )


def write_msbuild_props(artifact, out_dir):
    values = {
        "MaplibreNativeCMetadataPath": artifact["metadata_path"],
        "MaplibreNativeCLibraryPath": artifact["library_path"],
        "MaplibreNativeCImportLibraryPath": artifact["import_library_path"],
        "MaplibreNativeCIncludeDirs": path_list(artifact["include_dirs"]),
        "MaplibreNativeCLibraryDirs": path_list(artifact["library_dirs"]),
        "MaplibreNativeCLinkDirs": path_list(artifact["link_dirs"]),
        "MaplibreNativeCRuntimeLibraryDirs": path_list(
            artifact["runtime_library_dirs"]
        ),
        "MaplibreNativeCLoaderLibraryDirs": path_list(artifact["loader_library_dirs"]),
        "MaplibreNativeCLinkLibraries": path_list(artifact["link_libraries"]),
        "MaplibreNativeCFrameworks": path_list(artifact["frameworks"]),
        "MaplibreNativeCPkgConfigPath": str(out_dir),
    }
    lines = ["<Project>", "  <PropertyGroup>"]
    for key, value in values.items():
        lines.append(f"    <{key}>{xml_escape(value)}</{key}>")
    lines.extend(
        [
            "  </PropertyGroup>",
            "  <ItemGroup>",
            '    <RuntimeHostConfigurationOption Include="Maplibre.Native.LibraryPath" Value="$(MaplibreNativeCLibraryPath)" />',
            '    <RuntimeHostConfigurationOption Include="Maplibre.Native.LibraryDirs" Value="$(MaplibreNativeCLoaderLibraryDirs)" />',
            "  </ItemGroup>",
            "</Project>",
        ]
    )
    (out_dir / "Maplibre.Native.C.props").write_text(
        "\n".join(lines) + "\n", encoding="utf-8"
    )


def write_shell_env(artifact, out_dir):
    values = {
        "MAPLIBRE_NATIVE_FFI_LIBRARY_PATH": artifact["library_path"],
        "MLN_FFI_NATIVE_LIBRARY_PATH": artifact["library_path"],
        "MLN_FFI_NATIVE_IMPORT_LIBRARY_PATH": artifact["import_library_path"],
        "MLN_FFI_NATIVE_INCLUDE_DIRS": path_list(artifact["include_dirs"]),
        "MLN_FFI_NATIVE_LIBRARY_DIRS": path_list(artifact["loader_library_dirs"]),
        "MLN_FFI_NATIVE_LINK_DIRS": path_list(artifact["link_dirs"]),
        "MLN_FFI_NATIVE_RUNTIME_LIBRARY_DIRS": path_list(
            artifact["runtime_library_dirs"]
        ),
        "MLN_FFI_NATIVE_LINK_LIBRARIES": path_list(artifact["link_libraries"]),
        "MLN_FFI_NATIVE_FRAMEWORKS": path_list(artifact["frameworks"]),
        "MLN_FFI_NATIVE_SWIFT_LINKER_FLAGS_FILE": str(
            out_dir / f"{LIBRARY_NAME}.swift-linker-flags"
        ),
    }
    lines = [f"export {key}={shlex.quote(value)}" for key, value in values.items()]
    lines.append(
        f'export PKG_CONFIG_PATH={shlex.quote(str(out_dir))}${{PKG_CONFIG_PATH:+":$PKG_CONFIG_PATH"}}'
    )
    (out_dir / f"{LIBRARY_NAME}.env").write_text(
        "\n".join(lines) + "\n", encoding="utf-8"
    )


def write_swift_linker_flags(artifact, out_dir):
    (out_dir / f"{LIBRARY_NAME}.swift-linker-flags").write_text(
        "\n".join(linker_flags(artifact)) + "\n",
        encoding="utf-8",
    )


def write_zig_config(artifact, out_dir):
    values = {
        "render_backend": artifact["render_backend"],
        "artifact_shape": artifact["artifact_shape"],
        "library_path": artifact["library_path"],
        "import_library_path": artifact["import_library_path"],
        "include_dirs": path_list(artifact["include_dirs"]),
        "library_dirs": path_list(artifact["library_dirs"]),
        "link_dirs": path_list(artifact["link_dirs"]),
        "runtime_library_dirs": path_list(artifact["runtime_library_dirs"]),
        "link_libraries": path_list(artifact["link_libraries"]),
        "frameworks": path_list(artifact["frameworks"]),
    }
    lines = [f"{key}={value}" for key, value in values.items()]
    (out_dir / f"{LIBRARY_NAME}.zig-config").write_text(
        "\n".join(lines) + "\n",
        encoding="utf-8",
    )


def generate(metadata_path, out_dir):
    out_dir.mkdir(parents=True, exist_ok=True)
    artifact = load_artifact(metadata_path)
    write_pkg_config(artifact, out_dir)
    write_gradle_properties(artifact, out_dir)
    write_msbuild_props(artifact, out_dir)
    write_shell_env(artifact, out_dir)
    write_swift_linker_flags(artifact, out_dir)
    write_zig_config(artifact, out_dir)


def main():
    parser = argparse.ArgumentParser(
        description="Translate native artifact metadata for binding build systems."
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    generate_parser = subparsers.add_parser("generate")
    generate_parser.add_argument("--metadata", required=True)
    generate_parser.add_argument("--out-dir")

    args = parser.parse_args()
    metadata_path = Path(args.metadata)
    out_dir = Path(args.out_dir) if args.out_dir else metadata_path.parent

    if args.command == "generate":
        generate(metadata_path, out_dir)


if __name__ == "__main__":
    main()
