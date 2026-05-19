const std = @import("std");

const BuildOptions = struct {
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
    cmake_artifact_dir: std.Build.LazyPath,
    dependency_library_dir: ?std.Build.LazyPath,
    render_backend: RenderBackend,
};

const RenderBackend = enum {
    metal,
    vulkan,
};

fn configureSysroot(b: *std.Build, target: std.Build.ResolvedTarget) void {
    if (b.sysroot != null or target.result.os.tag.isDarwin()) return;

    const sysroot = b.graph.environ_map.get("MLN_FFI_SYSROOT") orelse return;
    if (sysroot.len == 0) return;

    b.sysroot = sysroot;
}

fn addDarwinSdkPaths(b: *std.Build, module: *std.Build.Module, target: std.Build.ResolvedTarget) void {
    if (!target.result.os.tag.isDarwin()) return;

    const sdkroot = b.graph.environ_map.get("SDKROOT") orelse return;
    if (sdkroot.len == 0) return;

    module.addSystemFrameworkPath(.{ .cwd_relative = b.pathJoin(&.{ sdkroot, "System", "Library", "Frameworks" }) });
    module.addSystemIncludePath(.{ .cwd_relative = b.pathJoin(&.{ sdkroot, "usr", "include" }) });
    module.addLibraryPath(.{ .cwd_relative = b.pathJoin(&.{ sdkroot, "usr", "lib" }) });
}

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

fn renderBackendName(render_backend: RenderBackend) []const u8 {
    return switch (render_backend) {
        .metal => "metal",
        .vulkan => "vulkan",
    };
}

fn dependencyLibraryDir(b: *std.Build) ?std.Build.LazyPath {
    return b.option(
        std.Build.LazyPath,
        "dependency-library-dir",
        "Directory containing backend dependency libraries such as Vulkan",
    );
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
        addDarwinSdkPaths(b, example.root_module, options.target);
        example.root_module.linkFramework("Metal", .{});
    } else if (options.render_backend == .vulkan) {
        example.root_module.addIncludePath(b.path("../../third_party/maplibre-native/vendor/Vulkan-Headers/include"));
        if (options.dependency_library_dir) |dependency_library_dir| {
            example.root_module.addLibraryPath(dependency_library_dir);
            example.root_module.addRPath(dependency_library_dir);
        }
        example.root_module.linkSystemLibrary(vulkanLibraryName(options.target), .{});
    }
    b.installArtifact(example);
    return example;
}

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    configureSysroot(b, target);
    const options = BuildOptions{
        .target = target,
        .optimize = b.standardOptimizeOption(.{}),
        .cmake_artifact_dir = cmakeArtifactDir(b),
        .dependency_library_dir = dependencyLibraryDir(b),
        .render_backend = renderBackend(b),
    };

    const readback = addReadbackExample(b, options);
    const run_readback = b.addRunArtifact(readback);
    if (b.args) |args| run_readback.addArgs(args);

    const run_step = b.step("run", "Render a map image to map.ppm");
    run_step.dependOn(&run_readback.step);
}
