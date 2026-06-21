const std = @import("std");
const maplibre_build = @import("maplibre_native");

const BuildOptions = struct {
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
    native_config_path: std.Build.LazyPath,
    include_dirs: []const std.Build.LazyPath,
    dependency_library_dirs: []const std.Build.LazyPath,
    render_backend: maplibre_build.RenderBackend,
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
    return options.render_backend == .opengl and options.target.result.os.tag == .windows;
}

fn sdlLibrary(b: *std.Build, options: BuildOptions) *std.Build.Step.Compile {
    const sdl = b.dependency("sdl", .{
        .target = options.target,
        .optimize = options.optimize,
    });
    return sdl.artifact("SDL3");
}

fn maplibreNativeModule(b: *std.Build, options: BuildOptions) *std.Build.Module {
    return maplibre_build.maplibreNativeModule(b, .{
        .target = options.target,
        .optimize = options.optimize,
        .native_config_path = options.native_config_path,
    });
}

fn addSdlTranslateC(b: *std.Build, module: *std.Build.Module, options: BuildOptions) void {
    if (!needsSdl(options)) return;
    const sdl = sdlLibrary(b, options);
    module.addImport("sdl", maplibre_build.translateCModule(b, .{
        .root_source_file = b.path("sdl_bindings.h"),
        .target = options.target,
        .optimize = options.optimize,
        .include_dirs = appendIncludeDir(b, options.include_dirs, sdl.getEmittedIncludeTree()),
        .c_macros = maplibre_build.sdlTranslateCMacros(options.target),
    }));
    module.linkLibrary(sdl);
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
    maplibre_build.addRenderBackendTranslateC(b, example.root_module, .{
        .target = options.target,
        .optimize = options.optimize,
        .include_dirs = options.include_dirs,
        .render_backend = options.render_backend,
    });
    addSdlTranslateC(b, example.root_module, options);
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
        .native_config_path = maplibre_build.nativeArtifactConfigPath(b),
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
