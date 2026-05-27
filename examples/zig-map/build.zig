const std = @import("std");
const maplibre_build = @import("maplibre_native");
const zigglgen = @import("zigglgen");

const BuildOptions = struct {
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
    cmake_artifact_dir: std.Build.LazyPath,
    include_dirs: []const std.Build.LazyPath,
    dependency_library_dir: ?std.Build.LazyPath,
    render_backend: maplibre_build.RenderBackend,
};

fn failUnsupportedTarget() noreturn {
    @panic("zig-map does not support this target platform");
}

fn maplibreNativeModule(b: *std.Build, options: BuildOptions) *std.Build.Module {
    return maplibre_build.maplibreNativeModule(b, .{
        .target = options.target,
        .optimize = options.optimize,
        .cmake_artifact_dir = options.cmake_artifact_dir,
        .include_dirs = options.include_dirs,
        .render_backend = options.render_backend,
        .dependency_library_dir = options.dependency_library_dir,
    });
}

fn addSdlTranslateCWorkarounds(module: *std.Build.Module, target: std.Build.ResolvedTarget) void {
    if (target.result.os.tag != .windows or target.result.abi != .msvc) return;

    // Zig 0.16 translate-c cannot parse MSVC's non-standard i64/ui64 integer
    // literal suffixes when SDL3 headers expose them through @cImport. Keep the
    // workaround local to zig-map: it is the only Zig target that imports SDL.
    module.addCMacro("SIZE_MAX", "((size_t)-1)");
    module.addCMacro("SDL_SINT64_C(c)", "c##LL");
    module.addCMacro("SDL_UINT64_C(c)", "c##ULL");
}

fn addZigMapExample(b: *std.Build, options: BuildOptions) *std.Build.Step.Compile {
    const root_module = b.createModule(.{
        .root_source_file = b.path("main.zig"),
        .target = options.target,
        .optimize = options.optimize,
    });
    const example = b.addExecutable(.{
        .name = "zig-map",
        .root_module = root_module,
    });

    maplibre_build.addRenderBackendOptions(b, root_module, options.render_backend);
    maplibre_build.addIncludePaths(root_module, options.include_dirs);
    addSdlTranslateCWorkarounds(root_module, options.target);
    root_module.addImport("maplibre_native", maplibreNativeModule(b, options));
    if (options.render_backend == .opengl) {
        const gl_bindings = zigglgen.generateBindingsModule(b, if (options.target.result.os.tag == .linux)
            .{ .api = .gles, .version = .@"3.0" }
        else
            .{ .api = .gl, .version = .@"3.0" });
        root_module.addImport("gl", gl_bindings);
    }
    if (options.dependency_library_dir) |dependency_library_dir| {
        root_module.addLibraryPath(dependency_library_dir);
        root_module.addRPath(dependency_library_dir);
    }
    root_module.linkSystemLibrary("SDL3", .{});
    maplibre_build.linkRenderBackend(b, root_module, .{
        .target = options.target,
        .render_backend = options.render_backend,
        .dependency_library_dir = options.dependency_library_dir,
    });

    if (options.render_backend == .metal) {
        const zig_objc = b.dependency("zig_objc", .{
            .target = options.target,
            .optimize = options.optimize,
        });
        root_module.addImport("objc", zig_objc.module("objc"));
        root_module.linkFramework("Foundation", .{});
    }

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
        .dependency_library_dir = maplibre_build.dependencyLibraryDir(b),
        .render_backend = render_backend,
    };

    const run_step = b.step("run", "Run Zig map example");
    if (!maplibre_build.isSupportedTarget(options.target, options.render_backend)) {
        failUnsupportedTarget();
    }
    const zig_map = addZigMapExample(b, options);
    const run_zig_map = b.addRunArtifact(zig_map);
    if (b.args) |args| run_zig_map.addArgs(args);
    run_step.dependOn(&run_zig_map.step);
}
