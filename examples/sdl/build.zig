const std = @import("std");
const Translator = @import("translate_c").Translator;

pub fn build(b: *std.Build) void {
    const optimize = b.standardOptimizeOption(.{});
    // A cross build for Windows names its MSVC libc installation, which the
    // example and SDL compile against. Host tools keep the host's libc.
    const target_libc = b.option(std.Build.LazyPath, "target-libc", "Target libc installation file");
    const maplibre = b.dependency("maplibre_native_ffi", .{
        .optimize = optimize,
        .@"local-build" = b.option(std.Build.LazyPath, "local-build", "Absolute path to a native install prefix"),
        .@"target-libc" = target_libc,
    });
    const maplibre_module = maplibre.module("maplibre_native_ffi");
    // The native library's install decides the target.
    const target = maplibre_module.resolved_target.?;
    switch (target.result.os.tag) {
        .linux, .freebsd, .windows, .macos => {},
        else => @panic("The SDL example runs on Linux and FreeBSD with Wayland, Windows, and macOS"),
    }

    const sdl = b.dependency("sdl", .{
        .target = target,
        .optimize = optimize,
    });
    const sdl_library = sdl.artifact("SDL3");
    sdl_library.setLibCFile(target_libc);
    const sdl_headers: Translator = .init(b.dependency("translate_c", .{}), .{
        .c_source_file = b.addWriteFiles().add("sdl.h",
            \\#include <SDL3/SDL.h>
            \\// The example enters SDL's main callbacks itself.
            \\#define SDL_MAIN_NOIMPL
            \\#include <SDL3/SDL_main.h>
            \\
        ),
        .target = target,
        .optimize = optimize,
        .libc_file = target_libc,
    });
    sdl_headers.addIncludePath(sdl.path("include"));

    const module = b.createModule(.{
        .root_source_file = b.path("src/main.zig"),
        .target = target,
        .optimize = optimize,
        .imports = &.{
            .{ .name = "maplibre_native_ffi", .module = maplibre_module },
            .{ .name = "sdl", .module = sdl_headers.mod },
        },
    });
    module.linkLibrary(sdl_library);
    if (target.result.os.tag == .windows) module.linkSystemLibrary("dwmapi", .{});

    const exe = b.addExecutable(.{
        .name = "maplibre-sdl-example",
        .root_module = module,
    });
    exe.setLibCFile(target_libc);
    // The MSVC runtime in a cross build's libc installation has no debug
    // information, which the linker would warn about at each of its objects.
    if (target_libc != null and target.result.abi == .msvc) exe.root_module.strip = true;
    b.installArtifact(exe);

    // The executable loads the native library from its own directory on
    // Windows, and from ../lib elsewhere.
    const libraries = if (target.result.os.tag == .windows) "bin" else "lib";
    b.installDirectory(.{
        .source_dir = maplibre.namedLazyPath("prefix").path(b, libraries),
        .install_dir = .{ .custom = libraries },
        .install_subdir = "",
        .include_extensions = &.{ "maplibre-native-c.dll", "libmaplibre-native-c.so", "libmaplibre-native-c.dylib" },
    });

    const run = b.addRunArtifact(exe);
    run.step.dependOn(b.getInstallStep());
    b.step("run", "Run the example").dependOn(&run.step);
}
