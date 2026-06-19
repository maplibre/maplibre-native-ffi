const std = @import("std");
const maplibre_build = @import("maplibre_native");
const zigglgen = @import("zigglgen");

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

fn sdlLibrary(b: *std.Build, options: BuildOptions) *std.Build.Step.Compile {
    const sdl = b.dependency("sdl", .{
        .target = options.target,
        .optimize = options.optimize,
    });
    const library = sdl.artifact("SDL3");
    if (options.target.result.os.tag == .macos) {
        if (b.graph.environ_map.get("MLN_FFI_SYSTEM_ROOT")) |system_root| {
            if (system_root.len != 0) {
                library.root_module.addSystemIncludePath(.{ .cwd_relative = b.pathJoin(&.{ system_root, "usr", "include" }) });
                library.root_module.addLibraryPath(.{ .cwd_relative = b.pathJoin(&.{ system_root, "usr", "lib" }) });
                library.root_module.addSystemFrameworkPath(.{ .cwd_relative = b.pathJoin(&.{ system_root, "System", "Library", "Frameworks" }) });
            }
        }
    }
    return library;
}

fn maplibreNativeModule(b: *std.Build, options: BuildOptions) *std.Build.Module {
    return maplibre_build.maplibreNativeModule(b, .{
        .target = options.target,
        .optimize = options.optimize,
        .native_config_path = options.native_config_path,
    });
}

fn cBindingsHeader(b: *std.Build, backend: maplibre_build.RenderBackend) std.Build.LazyPath {
    return switch (backend) {
        .metal => b.path("c_metal.h"),
        .opengl => b.path("c_opengl.h"),
        .vulkan => b.path("c_vulkan.h"),
    };
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

    const sdl = sdlLibrary(b, options);
    const c_include_dirs = appendIncludeDir(b, options.include_dirs, sdl.getEmittedIncludeTree());

    maplibre_build.addRenderBackendOptions(b, root_module, options.render_backend);
    maplibre_build.addIncludePaths(root_module, options.include_dirs);
    root_module.addImport("c", maplibre_build.translateCModule(b, .{
        .root_source_file = cBindingsHeader(b, options.render_backend),
        .target = options.target,
        .optimize = options.optimize,
        .include_dirs = c_include_dirs,
        .c_macros = maplibre_build.sdlTranslateCMacros(options.target),
    }));
    root_module.addImport("maplibre_native", maplibreNativeModule(b, options));
    if (options.render_backend == .opengl) {
        const gl_bindings = zigglgen.generateBindingsModule(b, if (options.target.result.os.tag == .linux or options.target.result.os.tag == .macos)
            .{ .api = .gles, .version = .@"3.0" }
        else
            .{ .api = .gl, .version = .@"3.0" });
        root_module.addImport("gl", gl_bindings);
    }
    for (options.dependency_library_dirs) |dependency_library_dir| {
        root_module.addLibraryPath(dependency_library_dir);
        root_module.addRPath(dependency_library_dir);
    }
    root_module.linkLibrary(sdl);
    maplibre_build.linkRenderBackend(b, root_module, .{
        .target = options.target,
        .render_backend = options.render_backend,
        .dependency_library_dirs = options.dependency_library_dirs,
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
        .native_config_path = maplibre_build.nativeArtifactConfigPath(b),
        .include_dirs = maplibre_build.includeDirs(b),
        .dependency_library_dirs = maplibre_build.dependencyLibraryDirs(b),
        .render_backend = render_backend,
    };

    const run_step = b.step("run", "Run Zig map example");
    const zig_map = addZigMapExample(b, options);
    const run_zig_map = b.addRunArtifact(zig_map);
    if (b.args) |args| run_zig_map.addArgs(args);
    run_step.dependOn(&run_zig_map.step);
}
