const std = @import("std");
const maplibre_build = @import("maplibre_native_ffi");

pub fn build(b: *std.Build) void {
    const native_install_dir = maplibre_build.nativeInstallDirPath(b);
    const target = maplibre_build.nativeTarget(b, native_install_dir);
    const optimize = b.standardOptimizeOption(.{});
    const dependency_include_dirs = maplibre_build.dependencyIncludeDirs(b);

    // The plugin builds as a shared library for the consumer loading path. Its
    // module gets only the translated headers and libc, never the link helper:
    // the register function arrives as a runtime argument, so the library
    // carries no undefined maplibre-native-c symbols.
    const library = b.addLibrary(.{
        .name = "maplibre-location-puck",
        .root_module = b.createModule(.{
            .root_source_file = b.path("src/plugin.zig"),
            .target = target,
            .optimize = optimize,
        }),
        .linkage = .dynamic,
    });
    maplibre_build.addMaplibreNativeIncludes(b, library.root_module, .{
        .include_dirs = maplibre_build.installedIncludeDirs(b, native_install_dir, dependency_include_dirs),
        .target = target,
        .optimize = optimize,
        .system_root = maplibre_build.maybeSystemRootPath(b),
    });
    b.installArtifact(library);
}
