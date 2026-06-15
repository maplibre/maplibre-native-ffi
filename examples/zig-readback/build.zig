const std = @import("std");
const maplibre_build = @import("maplibre_native");

const BuildOptions = struct {
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
    cmake_artifact_dir: std.Build.LazyPath,
    include_dirs: []const std.Build.LazyPath,
    dependency_library_dirs: []const std.Build.LazyPath,
    render_backend: maplibre_build.RenderBackend,
};

fn maplibreNativeModule(b: *std.Build, options: BuildOptions) *std.Build.Module {
    return maplibre_build.maplibreNativeModule(b, .{
        .target = options.target,
        .optimize = options.optimize,
        .cmake_artifact_dir = options.cmake_artifact_dir,
        .include_dirs = options.include_dirs,
        .render_backend = options.render_backend,
        .dependency_library_dirs = options.dependency_library_dirs,
    });
}

fn addSdlTranslateCWorkarounds(module: *std.Build.Module, target: std.Build.ResolvedTarget) void {
    if (target.result.os.tag != .windows or target.result.abi != .msvc) return;

    module.addCMacro("SIZE_MAX", "((size_t)-1)");
    module.addCMacro("SDL_SINT64_C(c)", "c##LL");
    module.addCMacro("SDL_UINT64_C(c)", "c##ULL");
}

fn addReadbackExample(b: *std.Build, options: BuildOptions) *std.Build.Step.Compile {
    const example = b.addExecutable(.{
        .name = "zig-readback",
        .root_module = b.createModule(.{
            .root_source_file = b.path("main.zig"),
            .target = options.target,
            .optimize = options.optimize,
        }),
    });

    maplibre_build.addRenderBackendOptions(b, example.root_module, options.render_backend);
    maplibre_build.addIncludePaths(example.root_module, options.include_dirs);
    if (options.render_backend == .opengl and options.target.result.os.tag == .windows) {
        addSdlTranslateCWorkarounds(example.root_module, options.target);
        for (options.dependency_library_dirs) |dependency_library_dir| {
            example.root_module.addLibraryPath(dependency_library_dir);
            example.root_module.addRPath(dependency_library_dir);
        }
        example.root_module.linkSystemLibrary("SDL3", .{});
    }
    example.root_module.addImport("maplibre_native", maplibreNativeModule(b, options));
    maplibre_build.linkRenderBackend(b, example.root_module, .{
        .target = options.target,
        .render_backend = options.render_backend,
        .dependency_library_dirs = options.dependency_library_dirs,
    });
    b.installArtifact(example);
    return example;
}

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const render_backend = maplibre_build.renderBackend(b);
    const options = BuildOptions{
        .target = target,
        .optimize = b.standardOptimizeOption(.{}),
        .cmake_artifact_dir = maplibre_build.cmakeArtifactDir(b),
        .include_dirs = maplibre_build.includeDirs(b),
        .dependency_library_dirs = maplibre_build.dependencyLibraryDirs(b),
        .render_backend = render_backend,
    };

    const readback = addReadbackExample(b, options);
    const run_readback = b.addRunArtifact(readback);
    if (b.args) |args| run_readback.addArgs(args);

    const run_step = b.step("run", "Render a map image to map.ppm");
    run_step.dependOn(&run_readback.step);
}
