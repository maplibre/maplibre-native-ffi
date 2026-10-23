const std = @import("std");
const maplibre_build = @import("maplibre_native_ffi");

const BuildOptions = struct {
    native: maplibre_build.Native,
    optimize: std.builtin.OptimizeMode,
    include_dirs: []const std.Build.LazyPath,
    dependency_include_dirs: []const std.Build.LazyPath,
    dependency_library_dirs: []const std.Build.LazyPath,
    system_root: ?std.Build.LazyPath,
};

fn appendIncludeDir(
    b: *std.Build,
    include_dirs: []const std.Build.LazyPath,
    include_dir: std.Build.LazyPath,
) []const std.Build.LazyPath {
    const result = b.allocator.alloc(std.Build.LazyPath, include_dirs.len + 1) catch @panic("out of memory");
    @memcpy(result[0..include_dirs.len], include_dirs);
    result[include_dirs.len] = include_dir;
    return result;
}

fn needsSdl(options: BuildOptions) bool {
    return options.native.backend == .opengl and options.native.target.result.os.tag == .windows;
}

fn sdlLibrary(b: *std.Build, options: BuildOptions) *std.Build.Step.Compile {
    const sdl = b.dependency("sdl", .{
        .target = options.native.target,
        .optimize = options.optimize,
    });
    return sdl.artifact("SDL3");
}

fn sdlTranslateCMacros(target: std.Build.ResolvedTarget) []const maplibre_build.CMacro {
    if (target.result.os.tag != .windows or target.result.abi != .msvc) return &.{};
    if (target.result.cpu.arch == .aarch64) {
        return &.{
            .{ .name = "SIZE_MAX", .value = "((size_t)-1)" },
            // Zig translate-c does not define __clang__, so Windows ARM64 UCRT
            // wchar.h selects NEON intrinsics without importing arm_neon.h.
            .{ .name = "_M_CEE", .value = "1" },
            .{ .name = "__clrcall", .value = "__cdecl" },
            .{ .name = "SDL_SINT64_C(c)", .value = "c##LL" },
            .{ .name = "SDL_UINT64_C(c)", .value = "c##ULL" },
        };
    }
    return &.{
        .{ .name = "SIZE_MAX", .value = "((size_t)-1)" },
        .{ .name = "SDL_SINT64_C(c)", .value = "c##LL" },
        .{ .name = "SDL_UINT64_C(c)", .value = "c##ULL" },
    };
}

fn maplibreNativeModule(b: *std.Build, options: BuildOptions) *std.Build.Module {
    return b.dependency("maplibre_native_ffi", .{
        .target = options.native.target,
        .optimize = options.optimize,
        .@"local-build" = options.native.prefix,
        .backend = options.native.backend,
        .@"dependency-include-dir" = options.dependency_include_dirs,
        .@"system-root" = options.system_root,
    }).module("maplibre_native_ffi");
}

fn vulkanBindingsHeader(b: *std.Build) std.Build.LazyPath {
    const header = b.addWriteFiles();
    return header.add("vulkan_bindings.h", "#include <vulkan/vulkan.h>\n");
}

fn eglBindingsHeader(b: *std.Build) std.Build.LazyPath {
    const header = b.addWriteFiles();
    return header.add("egl_bindings.h",
        \\#define EGL_EGLEXT_PROTOTYPES 1
        \\#include <EGL/egl.h>
        \\#include <EGL/eglext.h>
        \\#if __has_include(<EGL/eglext_angle.h>)
        \\#include <EGL/eglext_angle.h>
        \\#endif
        \\
    );
}

/// Imports the render backend's C API as `vulkan` or `egl`.
fn addRenderBackendTranslateC(b: *std.Build, module: *std.Build.Module, options: BuildOptions) void {
    const target = options.native.target;
    const name: []const u8, const header: std.Build.LazyPath = switch (options.native.backend) {
        .vulkan => .{ "vulkan", vulkanBindingsHeader(b) },
        .opengl => if (target.result.os.tag == .linux or target.result.os.tag == .macos)
            .{ "egl", eglBindingsHeader(b) }
        else
            return,
        .metal => return,
    };
    module.addImport(name, maplibre_build.translateCModule(b, .{
        .root_source_file = header,
        .target = target,
        .optimize = options.optimize,
        .include_dirs = options.include_dirs,
        .system_root = options.system_root,
    }));
}

fn addSdlTranslateC(b: *std.Build, module: *std.Build.Module, options: BuildOptions) void {
    if (!needsSdl(options)) return;
    const sdl = sdlLibrary(b, options);
    module.addImport("sdl", maplibre_build.translateCModule(b, .{
        .root_source_file = b.path("sdl_bindings.h"),
        .target = options.native.target,
        .optimize = options.optimize,
        .include_dirs = appendIncludeDir(b, options.include_dirs, sdl.getEmittedIncludeTree()),
        .c_macros = sdlTranslateCMacros(options.native.target),
        .system_root = options.system_root,
    }));
    module.linkLibrary(sdl);
}

fn addDependencyLibraryPaths(module: *std.Build.Module, dependency_library_dirs: []const std.Build.LazyPath) void {
    for (dependency_library_dirs) |dependency_library_dir| {
        module.addLibraryPath(dependency_library_dir);
        module.addRPath(dependency_library_dir);
    }
}

/// Finds the Vulkan loader on macOS and Windows, which have no system copy.
fn addNativeVulkanSdkPath(b: *std.Build, module: *std.Build.Module, options: BuildOptions) void {
    const target = options.native.target;
    if (options.dependency_library_dirs.len != 0) return;
    if (target.result.os.tag != .windows and target.result.os.tag != .macos) return;
    const library_dir = if (target.result.os.tag == .macos)
        b.graph.environ_map.get("MLN_FFI_VULKAN_LOADER_DIR") orelse
            if (b.graph.environ_map.get("VULKAN_SDK")) |sdk| b.pathJoin(&.{ sdk, "lib" }) else return
    else if (b.graph.environ_map.get("VULKAN_SDK")) |sdk|
        b.pathJoin(&.{ sdk, "Lib" })
    else
        @panic("native Windows Vulkan builds require VULKAN_SDK");
    module.addLibraryPath(.{ .cwd_relative = library_dir });
    module.addRPath(.{ .cwd_relative = library_dir });
}

/// Links the render backend's graphics libraries, which the example calls
/// itself.
fn linkRenderBackend(b: *std.Build, module: *std.Build.Module, options: BuildOptions) void {
    const target = options.native.target;
    maplibre_build.addPlatformSystemPaths(b, module, target, options.system_root);
    switch (options.native.backend) {
        .metal => {
            module.linkFramework("Metal", .{});
            module.linkFramework("QuartzCore", .{});
        },
        .opengl => switch (target.result.os.tag) {
            .linux, .macos => {
                addDependencyLibraryPaths(module, options.dependency_library_dirs);
                module.linkSystemLibrary("EGL", .{});
                module.linkSystemLibrary("GLESv2", .{});
            },
            .windows => module.linkSystemLibrary("opengl32", .{}),
            else => unreachable,
        },
        .vulkan => {
            addNativeVulkanSdkPath(b, module, options);
            addDependencyLibraryPaths(module, options.dependency_library_dirs);
            module.linkSystemLibrary(if (target.result.os.tag == .windows) "vulkan-1" else "vulkan", .{});
        },
    }
}

fn addReadbackExample(b: *std.Build, options: BuildOptions) *std.Build.Step.Compile {
    const example = b.addExecutable(.{
        .name = "zig-readback",
        .root_module = b.createModule(.{
            .root_source_file = b.path("main.zig"),
            .target = options.native.target,
            .optimize = options.optimize,
        }),
    });

    maplibre_build.addRenderBackendOptions(b, example.root_module, options.native.backend);
    addRenderBackendTranslateC(b, example.root_module, options);
    addSdlTranslateC(b, example.root_module, options);
    example.root_module.addImport("maplibre_native_ffi", maplibreNativeModule(b, options));
    linkRenderBackend(b, example.root_module, options);
    b.installArtifact(example);
    return example;
}

pub fn build(b: *std.Build) void {
    const native = maplibre_build.localNative(b);
    const dependency_include_dirs = b.option([]const std.Build.LazyPath, "dependency-include-dir", "Additional local dependency include directory") orelse &.{};
    const options = BuildOptions{
        .native = native,
        .optimize = b.standardOptimizeOption(.{}),
        .include_dirs = appendIncludeDir(b, dependency_include_dirs, native.prefix.path(b, "include")),
        .dependency_include_dirs = dependency_include_dirs,
        .dependency_library_dirs = b.option([]const std.Build.LazyPath, "dependency-library-dir", "Additional local dependency library directory") orelse &.{},
        .system_root = b.option(std.Build.LazyPath, "system-root", "Target platform SDK or sysroot"),
    };

    const readback = addReadbackExample(b, options);
    const run_readback = b.addRunArtifact(readback);
    run_readback.addPassthruArgs();

    const run_step = b.step("run", "Render a map image to map.ppm");
    run_step.dependOn(&run_readback.step);
}
