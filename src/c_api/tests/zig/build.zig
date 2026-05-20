const std = @import("std");
const maplibre_build = @import("maplibre_native");

const BuildOptions = struct {
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
    cmake_artifact_dir: std.Build.LazyPath,
    cmake_artifact_dir_runtime_path: []const u8,
    include_dirs: []const std.Build.LazyPath,
    dependency_library_dir: ?std.Build.LazyPath,
    render_backend: maplibre_build.RenderBackend,
};

fn addCTests(b: *std.Build, options: BuildOptions) *std.Build.Step.Compile {
    const c_tests = b.addTest(.{
        .root_module = b.createModule(.{
            .root_source_file = b.path("main.zig"),
            .target = options.target,
            .optimize = options.optimize,
        }),
    });
    maplibre_build.addRenderBackendOptions(b, c_tests.root_module, options.render_backend);
    maplibre_build.linkMaplibreNativeC(b, c_tests.root_module, .{
        .target = options.target,
        .cmake_artifact_dir = options.cmake_artifact_dir,
        .render_backend = options.render_backend,
        .include_dirs = options.include_dirs,
        .dependency_library_dir = options.dependency_library_dir,
    });
    return c_tests;
}

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const render_backend = maplibre_build.renderBackend(b);
    const cmake_artifact_dir = maplibre_build.cmakeArtifactDir(b);
    const options = BuildOptions{
        .target = target,
        .optimize = b.standardOptimizeOption(.{}),
        .cmake_artifact_dir = cmake_artifact_dir,
        .cmake_artifact_dir_runtime_path = cmake_artifact_dir.getPath2(b, null),
        .include_dirs = maplibre_build.includeDirs(b),
        .dependency_library_dir = maplibre_build.dependencyLibraryDir(b),
        .render_backend = render_backend,
    };

    const c_tests = addCTests(b, options);

    const run_c_tests = b.addRunArtifact(c_tests);
    if (target.result.os.tag == .windows) {
        run_c_tests.addPathDir(options.cmake_artifact_dir_runtime_path);
    }
    const test_step = b.step("test", "Run Zig C ABI tests");
    test_step.dependOn(&run_c_tests.step);
}
