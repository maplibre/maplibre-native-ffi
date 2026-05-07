const std = @import("std");

const BuildOptions = struct {
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
    cmake_artifact_dir: std.Build.LazyPath,
    render_backend: RenderBackend,
};

const RenderBackend = enum {
    metal,
    vulkan,
};

fn renderBackend(b: *std.Build, target: std.Build.ResolvedTarget) RenderBackend {
    const value = b.option(
        []const u8,
        "render-backend",
        "Render backend built into the CMake artifact: metal or vulkan",
    ) orelse switch (target.result.os.tag) {
        .macos, .ios => "metal",
        else => "vulkan",
    };

    if (std.mem.eql(u8, value, "metal")) return .metal;
    if (std.mem.eql(u8, value, "vulkan")) return .vulkan;
    std.debug.panic("unsupported render backend: {s}", .{value});
}

fn cmakeArtifactDir(b: *std.Build) std.Build.LazyPath {
    const path = b.option(
        []const u8,
        "cmake-artifact-dir",
        "Directory containing the CMake-built maplibre-native-c library",
    ) orelse "../../build/host";

    if (std.fs.path.isAbsolute(path)) {
        return .{ .cwd_relative = path };
    }
    return b.path(path);
}

fn linkMapLibreC(b: *std.Build, module: *std.Build.Module, cmake_artifact_dir: std.Build.LazyPath) void {
    module.addIncludePath(b.path("../../include"));
    module.addLibraryPath(cmake_artifact_dir);
    module.addRPath(cmake_artifact_dir);
    module.linkSystemLibrary("maplibre-native-c", .{});
    module.link_libc = true;
}

fn isSupportedTarget(options: BuildOptions) bool {
    return switch (options.render_backend) {
        .metal => options.target.result.os.tag == .macos,
        .vulkan => options.target.result.os.tag == .macos or options.target.result.os.tag == .linux,
    };
}

fn failUnsupportedTarget() noreturn {
    @panic("zig-map does not support this target platform");
}

fn addZigMapExample(b: *std.Build, options: BuildOptions) *std.Build.Step.Compile {
    const build_options = b.addOptions();
    build_options.addOption(bool, "supports_metal", options.render_backend == .metal);
    build_options.addOption(bool, "supports_vulkan", options.render_backend == .vulkan);

    const example = b.addExecutable(.{
        .name = "zig-map",
        .root_module = b.createModule(.{
            .root_source_file = b.path("main.zig"),
            .target = options.target,
            .optimize = options.optimize,
        }),
    });

    example.root_module.addOptions("build_options", build_options);
    linkMapLibreC(b, example.root_module, options.cmake_artifact_dir);
    example.root_module.addIncludePath(b.path("../../.pixi/envs/default/include"));
    example.root_module.addLibraryPath(b.path("../../.pixi/envs/default/lib"));
    example.root_module.addRPath(b.path("../../.pixi/envs/default/lib"));
    example.root_module.linkSystemLibrary("SDL3", .{});
    if (options.render_backend == .metal) {
        const zig_objc = b.dependency("zig_objc", .{
            .target = options.target,
            .optimize = options.optimize,
        });
        example.root_module.addImport("objc", zig_objc.module("objc"));
        example.root_module.linkFramework("Foundation", .{});
        example.root_module.linkFramework("Metal", .{});
        example.root_module.linkFramework("QuartzCore", .{});
    } else if (options.render_backend == .vulkan) {
        example.root_module.addIncludePath(b.path("../../third_party/maplibre-native/vendor/Vulkan-Headers/include"));
        example.root_module.linkSystemLibrary("vulkan", .{});
    } else {
        failUnsupportedTarget();
    }
    b.installArtifact(example);
    return example;
}

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const options = BuildOptions{
        .target = target,
        .optimize = b.standardOptimizeOption(.{}),
        .cmake_artifact_dir = cmakeArtifactDir(b),
        .render_backend = renderBackend(b, target),
    };

    const run_step = b.step("run", "Run Zig map example");
    if (!isSupportedTarget(options)) {
        failUnsupportedTarget();
    }
    const zig_map = addZigMapExample(b, options);
    const run_zig_map = b.addRunArtifact(zig_map);
    if (b.args) |args| run_zig_map.addArgs(args);
    run_step.dependOn(&run_zig_map.step);
}
