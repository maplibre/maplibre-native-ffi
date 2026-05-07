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

fn addReadbackExample(b: *std.Build, options: BuildOptions) *std.Build.Step.Compile {
    const build_options = b.addOptions();
    build_options.addOption(bool, "supports_metal", options.render_backend == .metal);
    build_options.addOption(bool, "supports_vulkan", options.render_backend == .vulkan);

    const example = b.addExecutable(.{
        .name = "zig-readback",
        .root_module = b.createModule(.{
            .root_source_file = b.path("main.zig"),
            .target = options.target,
            .optimize = options.optimize,
        }),
    });

    example.root_module.addOptions("build_options", build_options);
    linkMapLibreC(b, example.root_module, options.cmake_artifact_dir);
    example.root_module.addLibraryPath(b.path("../../.pixi/envs/default/lib"));
    example.root_module.addRPath(b.path("../../.pixi/envs/default/lib"));
    if (options.render_backend == .vulkan) {
        example.root_module.addIncludePath(b.path("../../third_party/maplibre-native/vendor/Vulkan-Headers/include"));
        example.root_module.linkSystemLibrary("vulkan", .{});
    } else if (options.render_backend == .metal) {
        example.root_module.linkFramework("Metal", .{});
        example.root_module.linkFramework("QuartzCore", .{});
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

    const readback = addReadbackExample(b, options);
    const run_readback = b.addRunArtifact(readback);
    if (b.args) |args| run_readback.addArgs(args);

    const run_step = b.step("run", "Render a map image to map.ppm");
    run_step.dependOn(&run_readback.step);
}
