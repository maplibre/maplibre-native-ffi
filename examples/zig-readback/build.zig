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
    ) orelse b.path("../../build/host");
}

fn renderBackendName(render_backend: RenderBackend) []const u8 {
    return switch (render_backend) {
        .metal => "metal",
        .vulkan => "vulkan",
    };
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

fn addMaplibreNativeModule(b: *std.Build, options: BuildOptions) *std.Build.Module {
    const dependency = b.dependency("maplibre_native", .{
        .target = options.target,
        .optimize = options.optimize,
        .@"cmake-artifact-dir" = options.cmake_artifact_dir,
        .@"render-backend" = renderBackendName(options.render_backend),
    });
    return dependency.module("maplibre_native");
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
    example.root_module.addImport("maplibre_native", addMaplibreNativeModule(b, options));
    if (options.render_backend == .metal) {
        example.root_module.linkFramework("Metal", .{});
    } else if (options.render_backend == .vulkan) {
        example.root_module.addIncludePath(b.path("../../third_party/maplibre-native/vendor/Vulkan-Headers/include"));
        example.root_module.addLibraryPath(pixiLibraryDir(b, options.target));
        example.root_module.addRPath(pixiLibraryDir(b, options.target));
        example.root_module.linkSystemLibrary(vulkanLibraryName(options.target), .{});
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
        .render_backend = renderBackend(b),
    };

    const readback = addReadbackExample(b, options);
    const run_readback = b.addRunArtifact(readback);
    if (target.result.os.tag == .windows) {
        run_readback.addPathDir("../../.pixi/envs/default");
        run_readback.addPathDir("../../.pixi/envs/default/Library/bin");
    }
    if (b.args) |args| run_readback.addArgs(args);

    const run_step = b.step("run", "Render a map image to map.ppm");
    run_step.dependOn(&run_readback.step);
}
