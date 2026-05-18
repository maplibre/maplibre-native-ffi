const std = @import("std");

const BuildOptions = struct {
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
    cmake_artifact_dir_path: []const u8,
    cmake_artifact_dir: std.Build.LazyPath,
    dependency_library_dir: ?std.Build.LazyPath,
    render_backend: RenderBackend,
};

const RenderBackend = enum {
    metal,
    vulkan,
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

fn linkMapLibreC(b: *std.Build, module: *std.Build.Module, cmake_artifact_dir: std.Build.LazyPath) void {
    module.addIncludePath(b.path("../../../../include"));
    module.addLibraryPath(cmake_artifact_dir);
    module.addRPath(cmake_artifact_dir);
    module.linkSystemLibrary("maplibre-native-c", .{});
    module.link_libc = true;
}

fn vulkanLibraryName(target: std.Build.ResolvedTarget) []const u8 {
    return switch (target.result.os.tag) {
        .windows => "vulkan-1",
        else => "vulkan",
    };
}

fn dependencyLibraryDir(b: *std.Build) ?std.Build.LazyPath {
    return b.option(
        std.Build.LazyPath,
        "dependency-library-dir",
        "Directory containing backend dependency libraries such as Vulkan",
    );
}

fn lazyPath(b: *std.Build, path: []const u8) std.Build.LazyPath {
    if (std.fs.path.isAbsolute(path)) {
        return .{ .cwd_relative = path };
    }
    return b.path(path);
}

fn addCTests(b: *std.Build, options: BuildOptions) *std.Build.Step.Compile {
    const build_options = b.addOptions();
    build_options.addOption(bool, "supports_metal", options.render_backend == .metal);
    build_options.addOption(bool, "supports_vulkan", options.render_backend == .vulkan);

    const c_tests = b.addTest(.{
        .root_module = b.createModule(.{
            .root_source_file = b.path("main.zig"),
            .target = options.target,
            .optimize = options.optimize,
        }),
    });

    c_tests.root_module.addOptions("build_options", build_options);
    linkMapLibreC(b, c_tests.root_module, options.cmake_artifact_dir);
    if (options.render_backend == .metal) {
        c_tests.root_module.linkFramework("Metal", .{});
        c_tests.root_module.linkFramework("QuartzCore", .{});
    } else if (options.render_backend == .vulkan) {
        c_tests.root_module.addIncludePath(b.path("../../../../third_party/maplibre-native/vendor/Vulkan-Headers/include"));
        if (options.dependency_library_dir) |dependency_library_dir| {
            c_tests.root_module.addLibraryPath(dependency_library_dir);
            c_tests.root_module.addRPath(dependency_library_dir);
        }
        c_tests.root_module.linkSystemLibrary(vulkanLibraryName(options.target), .{});
    }
    return c_tests;
}

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const cmake_artifact_dir_path = b.option(
        []const u8,
        "cmake-artifact-dir",
        "Directory containing the CMake-built maplibre-native-c library",
    ) orelse "../../../../build";
    const options = BuildOptions{
        .target = target,
        .optimize = b.standardOptimizeOption(.{}),
        .cmake_artifact_dir_path = cmake_artifact_dir_path,
        .cmake_artifact_dir = lazyPath(b, cmake_artifact_dir_path),
        .dependency_library_dir = dependencyLibraryDir(b),
        .render_backend = renderBackend(b),
    };

    const c_tests = addCTests(b, options);

    const run_c_tests = b.addRunArtifact(c_tests);
    if (target.result.os.tag == .windows) {
        run_c_tests.addPathDir(options.cmake_artifact_dir_path);
    }
    const test_step = b.step("test", "Run Zig C ABI tests");
    test_step.dependOn(&run_c_tests.step);
}
