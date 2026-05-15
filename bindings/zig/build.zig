const std = @import("std");

const BuildOptions = struct {
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
    cmake_artifact_dir: std.Build.LazyPath,
    cmake_artifact_dir_runtime_path: []const u8,
    include_dir: std.Build.LazyPath,
    vulkan_include_dir: std.Build.LazyPath,
    dependency_library_dir: std.Build.LazyPath,
    render_backend: RenderBackend,
};

pub const RenderBackend = enum {
    metal,
    vulkan,
};

pub const LinkOptions = struct {
    target: std.Build.ResolvedTarget,
    cmake_artifact_dir: std.Build.LazyPath,
    render_backend: RenderBackend,
    include_dir: std.Build.LazyPath,
    vulkan_include_dir: ?std.Build.LazyPath = null,
    dependency_library_dir: ?std.Build.LazyPath = null,
};

fn renderBackend(b: *std.Build) RenderBackend {
    const value = b.option(
        []const u8,
        "render-backend",
        "Render backend built into the CMake artifact: metal or vulkan",
    ) orelse @panic("missing required -Drender-backend=metal|vulkan");

    if (std.mem.eql(u8, value, "metal")) return .metal;
    if (std.mem.eql(u8, value, "vulkan")) return .vulkan;
    std.debug.panic("unsupported render backend: {s}", .{value});
}

fn cmakeArtifactDir(b: *std.Build) std.Build.LazyPath {
    return b.option(
        std.Build.LazyPath,
        "cmake-artifact-dir",
        "Directory containing the CMake-built maplibre-native-c library",
    ) orelse @panic("missing required -Dcmake-artifact-dir=<path-to-cmake-artifacts>");
}

fn includeDir(b: *std.Build) std.Build.LazyPath {
    return b.option(
        std.Build.LazyPath,
        "include-dir",
        "Directory containing maplibre_native_c.h",
    ) orelse b.path("../../include");
}

fn vulkanIncludeDir(b: *std.Build) std.Build.LazyPath {
    return b.option(
        std.Build.LazyPath,
        "vulkan-include-dir",
        "Directory containing Vulkan headers for Vulkan builds",
    ) orelse b.path("../../third_party/maplibre-native/vendor/Vulkan-Headers/include");
}

fn dependencyLibraryDir(b: *std.Build, target: std.Build.ResolvedTarget) std.Build.LazyPath {
    const override = b.option(
        std.Build.LazyPath,
        "dependency-library-dir",
        "Directory containing backend dependency libraries such as Vulkan",
    );
    if (override) |path| return path;
    return pixiLibraryDir(b, target);
}

fn pixiLibraryDir(b: *std.Build, target: std.Build.ResolvedTarget) std.Build.LazyPath {
    return switch (target.result.os.tag) {
        .windows => b.path("../../.pixi/envs/default/Library/lib"),
        else => b.path("../../.pixi/envs/default/lib"),
    };
}

fn vulkanLibraryName(target: std.Build.ResolvedTarget) []const u8 {
    return switch (target.result.os.tag) {
        .windows => "vulkan-1",
        else => "vulkan",
    };
}

fn isSupportedTarget(target: std.Build.ResolvedTarget, render_backend: RenderBackend) bool {
    return switch (render_backend) {
        .metal => target.result.os.tag == .macos,
        .vulkan => target.result.os.tag == .macos or target.result.os.tag == .linux or
            target.result.os.tag == .windows,
    };
}

fn checkSupportedTarget(target: std.Build.ResolvedTarget, render_backend: RenderBackend) void {
    if (!isSupportedTarget(target, render_backend)) {
        std.debug.panic(
            "unsupported target/render-backend combination: {s}/{s}",
            .{ @tagName(target.result.os.tag), @tagName(render_backend) },
        );
    }
}

fn repoLinkOptions(_: *std.Build, options: BuildOptions) LinkOptions {
    return .{
        .target = options.target,
        .cmake_artifact_dir = options.cmake_artifact_dir,
        .render_backend = options.render_backend,
        .include_dir = options.include_dir,
        .vulkan_include_dir = options.vulkan_include_dir,
        .dependency_library_dir = options.dependency_library_dir,
    };
}

/// Links a Zig module to the MapLibre Native C library and its backend-specific dependencies.
///
/// Callers provide all filesystem paths explicitly so the helper works both from this
/// package and from external consumers with a different build root layout.
pub fn linkMaplibreNativeC(_: *std.Build, module: *std.Build.Module, options: LinkOptions) void {
    checkSupportedTarget(options.target, options.render_backend);

    module.addIncludePath(options.include_dir);
    module.addLibraryPath(options.cmake_artifact_dir);
    module.addRPath(options.cmake_artifact_dir);
    module.linkSystemLibrary("maplibre-native-c", .{});
    module.link_libc = true;

    switch (options.render_backend) {
        .metal => {
            module.linkFramework("Metal", .{});
            module.linkFramework("QuartzCore", .{});
        },
        .vulkan => {
            const vulkan_include_dir = options.vulkan_include_dir orelse
                @panic("missing LinkOptions.vulkan_include_dir for vulkan backend");
            module.addIncludePath(vulkan_include_dir);
            if (options.dependency_library_dir) |dependency_library_dir| {
                module.addLibraryPath(dependency_library_dir);
                module.addRPath(dependency_library_dir);
            }
            module.linkSystemLibrary(vulkanLibraryName(options.target), .{});
        },
    }
}

fn addMaplibreNativeModule(b: *std.Build, options: BuildOptions) *std.Build.Module {
    const module = b.addModule("maplibre_native", .{
        .root_source_file = b.path("src/maplibre_native.zig"),
        .target = options.target,
        .optimize = options.optimize,
    });
    linkMaplibreNativeC(b, module, repoLinkOptions(b, options));
    return module;
}

fn addBindingTests(b: *std.Build, options: BuildOptions, maplibre_native: *std.Build.Module) *std.Build.Step.Compile {
    const build_options = b.addOptions();
    build_options.addOption(bool, "supports_metal", options.render_backend == .metal);
    build_options.addOption(bool, "supports_vulkan", options.render_backend == .vulkan);

    const tests = b.addTest(.{
        .root_module = b.createModule(.{
            .root_source_file = b.path("tests/main.zig"),
            .target = options.target,
            .optimize = options.optimize,
        }),
    });
    tests.root_module.addImport("maplibre_native", maplibre_native);
    tests.root_module.addOptions("build_options", build_options);
    linkMaplibreNativeC(b, tests.root_module, repoLinkOptions(b, options));
    if (options.render_backend == .metal) {
        tests.root_module.addCSourceFile(.{ .file = b.path("tests/metal_support_macos.m") });
        tests.root_module.linkFramework("AppKit", .{});
    }
    return tests;
}

fn addPrivateModuleTests(
    b: *std.Build,
    options: BuildOptions,
    root_source_file: std.Build.LazyPath,
) *std.Build.Step.Compile {
    const tests = b.addTest(.{
        .root_module = b.createModule(.{
            .root_source_file = root_source_file,
            .target = options.target,
            .optimize = options.optimize,
        }),
    });
    linkMaplibreNativeC(b, tests.root_module, repoLinkOptions(b, options));
    return tests;
}

fn addWindowsTestRuntimePaths(run: *std.Build.Step.Run, options: BuildOptions) void {
    run.addPathDir(options.cmake_artifact_dir_runtime_path);
    run.addPathDir("../../.pixi/envs/default");
    run.addPathDir("../../.pixi/envs/default/Library/bin");
}

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const cmake_artifact_dir = cmakeArtifactDir(b);
    const options = BuildOptions{
        .target = target,
        .optimize = b.standardOptimizeOption(.{}),
        .cmake_artifact_dir = cmake_artifact_dir,
        .cmake_artifact_dir_runtime_path = cmake_artifact_dir.getPath2(b, null),
        .include_dir = includeDir(b),
        .vulkan_include_dir = vulkanIncludeDir(b),
        .dependency_library_dir = dependencyLibraryDir(b, target),
        .render_backend = renderBackend(b),
    };
    checkSupportedTarget(options.target, options.render_backend);

    const maplibre_native = addMaplibreNativeModule(b, options);
    const binding_tests = addBindingTests(b, options, maplibre_native);
    const status_tests = addPrivateModuleTests(b, options, b.path("src/status.zig"));
    const runtime_tests = addPrivateModuleTests(b, options, b.path("src/runtime.zig"));
    const logging_tests = addPrivateModuleTests(b, options, b.path("src/logging.zig"));
    const map_tests = addPrivateModuleTests(b, options, b.path("src/map.zig"));
    b.default_step.dependOn(&binding_tests.step);
    b.default_step.dependOn(&status_tests.step);
    b.default_step.dependOn(&runtime_tests.step);
    b.default_step.dependOn(&logging_tests.step);
    b.default_step.dependOn(&map_tests.step);

    const run_binding_tests = b.addRunArtifact(binding_tests);
    const run_status_tests = b.addRunArtifact(status_tests);
    const run_runtime_tests = b.addRunArtifact(runtime_tests);
    const run_logging_tests = b.addRunArtifact(logging_tests);
    const run_map_tests = b.addRunArtifact(map_tests);
    if (target.result.os.tag == .windows) {
        addWindowsTestRuntimePaths(run_binding_tests, options);
        addWindowsTestRuntimePaths(run_status_tests, options);
        addWindowsTestRuntimePaths(run_runtime_tests, options);
        addWindowsTestRuntimePaths(run_logging_tests, options);
        addWindowsTestRuntimePaths(run_map_tests, options);
    }

    const test_step = b.step("test", "Run Zig binding tests");
    test_step.dependOn(&run_binding_tests.step);
    test_step.dependOn(&run_status_tests.step);
    test_step.dependOn(&run_runtime_tests.step);
    test_step.dependOn(&run_logging_tests.step);
    test_step.dependOn(&run_map_tests.step);
}
