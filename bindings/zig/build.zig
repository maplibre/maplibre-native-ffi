const std = @import("std");

const BuildOptions = struct {
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
    cmake_artifact_dir: std.Build.LazyPath,
    include_dirs: []const std.Build.LazyPath,
    dependency_library_dir: ?std.Build.LazyPath,
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
    include_dirs: []const std.Build.LazyPath,
    dependency_library_dir: ?std.Build.LazyPath = null,
};

pub const DependencyOptions = struct {
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
    cmake_artifact_dir: std.Build.LazyPath,
    include_dirs: []const std.Build.LazyPath,
    render_backend: RenderBackend,
    dependency_library_dir: ?std.Build.LazyPath = null,
};

pub const RenderBackendLinkOptions = struct {
    target: std.Build.ResolvedTarget,
    render_backend: RenderBackend,
    dependency_library_dir: ?std.Build.LazyPath = null,
};

pub fn renderBackend(b: *std.Build) RenderBackend {
    return b.option(
        RenderBackend,
        "render-backend",
        "Render backend built into the CMake artifact: metal or vulkan",
    ) orelse @panic("missing required -Drender-backend=metal|vulkan");
}

pub fn cmakeArtifactDir(b: *std.Build) std.Build.LazyPath {
    return b.option(
        std.Build.LazyPath,
        "cmake-artifact-dir",
        "Directory containing the CMake-built maplibre-native-c library",
    ) orelse @panic("missing required -Dcmake-artifact-dir=<path-to-cmake-artifacts>");
}

pub fn includeDirs(b: *std.Build) []const std.Build.LazyPath {
    return b.option(
        []const std.Build.LazyPath,
        "include-dir",
        "Include directory. Repeat for project, dependency, and backend headers.",
    ) orelse @panic("missing required -Dinclude-dir=<path>; repeat for additional include roots");
}

pub fn addIncludePaths(module: *std.Build.Module, include_dirs: []const std.Build.LazyPath) void {
    for (include_dirs) |include_dir| {
        module.addIncludePath(include_dir);
    }
}

pub fn dependencyLibraryDir(b: *std.Build) ?std.Build.LazyPath {
    return b.option(
        std.Build.LazyPath,
        "dependency-library-dir",
        "Directory containing backend dependency libraries such as Vulkan",
    );
}

pub fn addPlatformSystemPaths(b: *std.Build, module: *std.Build.Module, target: std.Build.ResolvedTarget) void {
    if (!target.result.os.tag.isDarwin() and target.result.os.tag != .linux) return;
    const system_root = b.graph.environ_map.get("MLN_FFI_SYSTEM_ROOT") orelse return;
    if (system_root.len == 0) return;

    if (target.result.os.tag.isDarwin()) {
        module.addSystemFrameworkPath(.{ .cwd_relative = b.pathJoin(&.{ system_root, "System", "Library", "Frameworks" }) });
    }
    module.addSystemIncludePath(.{ .cwd_relative = b.pathJoin(&.{ system_root, "usr", "include" }) });
    module.addLibraryPath(.{ .cwd_relative = b.pathJoin(&.{ system_root, "usr", "lib" }) });
    if (target.result.os.tag == .linux) {
        module.addLibraryPath(.{ .cwd_relative = b.pathJoin(&.{ system_root, "usr", "lib64" }) });
        module.addLibraryPath(.{ .cwd_relative = b.pathJoin(&.{ system_root, "lib" }) });
        module.addLibraryPath(.{ .cwd_relative = b.pathJoin(&.{ system_root, "lib64" }) });
    }
}

pub fn vulkanLibraryName(target: std.Build.ResolvedTarget) []const u8 {
    return switch (target.result.os.tag) {
        .windows => "vulkan-1",
        else => "vulkan",
    };
}

pub fn isSupportedTarget(target: std.Build.ResolvedTarget, backend: RenderBackend) bool {
    return switch (backend) {
        .metal => target.result.os.tag == .macos,
        .vulkan => target.result.os.tag == .macos or target.result.os.tag == .linux or
            target.result.os.tag == .windows,
    };
}

pub fn checkSupportedTarget(target: std.Build.ResolvedTarget, backend: RenderBackend) void {
    if (!isSupportedTarget(target, backend)) {
        std.debug.panic(
            "unsupported target/render-backend combination: {s}/{s}",
            .{ @tagName(target.result.os.tag), @tagName(backend) },
        );
    }
}

pub fn addRenderBackendOptions(b: *std.Build, module: *std.Build.Module, backend: RenderBackend) void {
    const build_options = b.addOptions();
    build_options.addOption(bool, "supports_metal", backend == .metal);
    build_options.addOption(bool, "supports_vulkan", backend == .vulkan);
    module.addOptions("build_options", build_options);
}

pub fn linkRenderBackend(b: *std.Build, module: *std.Build.Module, options: RenderBackendLinkOptions) void {
    checkSupportedTarget(options.target, options.render_backend);
    addPlatformSystemPaths(b, module, options.target);

    switch (options.render_backend) {
        .metal => {
            module.linkFramework("Metal", .{});
            module.linkFramework("QuartzCore", .{});
        },
        .vulkan => {
            if (options.dependency_library_dir) |dependency_library_dir| {
                module.addLibraryPath(dependency_library_dir);
                module.addRPath(dependency_library_dir);
            }
            module.linkSystemLibrary(vulkanLibraryName(options.target), .{});
        },
    }
}

fn dependencyArgs(options: DependencyOptions) struct {
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
    @"cmake-artifact-dir": std.Build.LazyPath,
    @"include-dir": []const std.Build.LazyPath,
    @"render-backend": RenderBackend,
    @"dependency-library-dir": ?std.Build.LazyPath,
} {
    return .{
        .target = options.target,
        .optimize = options.optimize,
        .@"cmake-artifact-dir" = options.cmake_artifact_dir,
        .@"include-dir" = options.include_dirs,
        .@"render-backend" = options.render_backend,
        .@"dependency-library-dir" = options.dependency_library_dir,
    };
}

pub fn dependency(b: *std.Build, options: DependencyOptions) *std.Build.Dependency {
    return b.dependencyFromBuildZig(@This(), dependencyArgs(options));
}

pub fn maplibreNativeModule(b: *std.Build, options: DependencyOptions) *std.Build.Module {
    return dependency(b, options).module("maplibre_native");
}

fn repoLinkOptions(options: BuildOptions) LinkOptions {
    return .{
        .target = options.target,
        .cmake_artifact_dir = options.cmake_artifact_dir,
        .render_backend = options.render_backend,
        .include_dirs = options.include_dirs,
        .dependency_library_dir = options.dependency_library_dir,
    };
}

/// Links a Zig module to the MapLibre Native C library and its backend-specific dependencies.
///
/// Callers provide all filesystem paths explicitly so the helper works both from this
/// package and from external consumers with a different build root layout.
pub fn linkMaplibreNativeC(b: *std.Build, module_: *std.Build.Module, options: LinkOptions) void {
    addIncludePaths(module_, options.include_dirs);
    if (options.target.result.os.tag == .windows) {
        module_.addObjectFile(options.cmake_artifact_dir.path(b, "maplibre-native-c.lib"));
    } else {
        module_.addLibraryPath(options.cmake_artifact_dir);
        module_.addRPath(options.cmake_artifact_dir);
        module_.linkSystemLibrary("maplibre-native-c", .{});
    }
    module_.link_libc = true;
    linkRenderBackend(b, module_, .{
        .target = options.target,
        .render_backend = options.render_backend,
        .dependency_library_dir = options.dependency_library_dir,
    });
}

fn addMaplibreNativeModule(b: *std.Build, options: BuildOptions) *std.Build.Module {
    const maplibre_native = b.addModule("maplibre_native", .{
        .root_source_file = b.path("src/maplibre_native.zig"),
        .target = options.target,
        .optimize = options.optimize,
    });
    linkMaplibreNativeC(b, maplibre_native, repoLinkOptions(options));
    return maplibre_native;
}

fn addTestCompile(b: *std.Build, options: BuildOptions, root_source_file: std.Build.LazyPath) *std.Build.Step.Compile {
    const tests = b.addTest(.{
        .root_module = b.createModule(.{
            .root_source_file = root_source_file,
            .target = options.target,
            .optimize = options.optimize,
        }),
    });
    linkMaplibreNativeC(b, tests.root_module, repoLinkOptions(options));
    return tests;
}

fn addBindingTests(b: *std.Build, options: BuildOptions, maplibre_native: *std.Build.Module) *std.Build.Step.Compile {
    const tests = addTestCompile(b, options, b.path("tests/main.zig"));
    tests.root_module.addImport("maplibre_native", maplibre_native);
    addRenderBackendOptions(b, tests.root_module, options.render_backend);
    if (options.render_backend == .metal) {
        tests.root_module.addCSourceFile(.{ .file = b.path("tests/metal_support_macos.m") });
        tests.root_module.linkFramework("AppKit", .{});
    }
    return tests;
}

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const backend = renderBackend(b);
    const cmake_artifact_dir = cmakeArtifactDir(b);
    const options = BuildOptions{
        .target = target,
        .optimize = b.standardOptimizeOption(.{}),
        .cmake_artifact_dir = cmake_artifact_dir,
        .include_dirs = includeDirs(b),
        .dependency_library_dir = dependencyLibraryDir(b),
        .render_backend = backend,
    };
    checkSupportedTarget(options.target, options.render_backend);

    const maplibre_native = addMaplibreNativeModule(b, options);

    const test_sources = [_]std.Build.LazyPath{
        b.path("src/status.zig"),
        b.path("src/runtime.zig"),
        b.path("src/logging.zig"),
        b.path("src/map.zig"),
    };

    const test_step = b.step("test", "Run Zig binding tests");

    const binding_tests = addBindingTests(b, options, maplibre_native);
    b.default_step.dependOn(&binding_tests.step);
    const run_binding_tests = b.addRunArtifact(binding_tests);
    test_step.dependOn(&run_binding_tests.step);

    for (test_sources) |source| {
        const tests = addTestCompile(b, options, source);
        b.default_step.dependOn(&tests.step);
        const run_tests = b.addRunArtifact(tests);
        test_step.dependOn(&run_tests.step);
    }
}
