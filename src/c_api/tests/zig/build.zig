const std = @import("std");
const maplibre_build = @import("maplibre_native");
const zigglgen = @import("zigglgen");

const BuildOptions = struct {
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
    cmake_artifact_dir: std.Build.LazyPath,
    include_dirs: []const std.Build.LazyPath,
    dependency_library_dirs: []const std.Build.LazyPath,
    render_backend: maplibre_build.RenderBackend,
};

fn addCTests(b: *std.Build, options: BuildOptions) *std.Build.Step.Compile {
    const c_tests = b.addTest(.{
        .root_module = b.createModule(.{
            .root_source_file = b.path("main.zig"),
            .target = options.target,
            .optimize = options.optimize,
        }),
        .use_lld = if (isIosSimulator(options.target)) false else null,
    });
    maplibre_build.addRenderBackendOptions(b, c_tests.root_module, options.render_backend);
    if (isIosSimulator(options.target)) {
        c_tests.root_module.addCSourceFile(.{ .file = b.path("../../../zig_test_support/ios_simulator_dyld_stub.m") });
    }
    if (options.target.result.os.tag == .windows or options.target.result.os.tag == .linux or options.target.result.os.tag == .macos) {
        const gl_bindings = zigglgen.generateBindingsModule(b, if (options.target.result.os.tag == .linux or options.target.result.os.tag == .macos)
            .{ .api = .gles, .version = .@"3.0" }
        else
            .{ .api = .gl, .version = .@"3.0" });
        c_tests.root_module.addImport("gl", gl_bindings);
        if (options.target.result.os.tag == .windows) {
            const wgl_test_context = b.createModule(.{
                .root_source_file = b.path("../../../zig_test_support/wgl_context.zig"),
                .target = options.target,
                .optimize = options.optimize,
            });
            wgl_test_context.addImport("gl", gl_bindings);
            c_tests.root_module.addImport("wgl_test_context", wgl_test_context);
        }
    }
    maplibre_build.linkMaplibreNativeC(b, c_tests.root_module, .{
        .target = options.target,
        .cmake_artifact_dir = options.cmake_artifact_dir,
        .render_backend = options.render_backend,
        .include_dirs = options.include_dirs,
        .dependency_library_dirs = options.dependency_library_dirs,
    });
    return c_tests;
}

fn isIosSimulator(target: std.Build.ResolvedTarget) bool {
    return target.result.os.tag == .ios and target.result.abi == .simulator;
}

fn testOptimize(target: std.Build.ResolvedTarget, optimize: std.builtin.OptimizeMode) std.builtin.OptimizeMode {
    if (isIosSimulator(target) and optimize == .Debug) return .ReleaseSafe;
    return optimize;
}

fn addTestRunStep(b: *std.Build, tests: *std.Build.Step.Compile, target: std.Build.ResolvedTarget) *std.Build.Step.Run {
    if (isIosSimulator(target)) {
        const run_tests = b.addSystemCommand(&.{
            "bash",
            b.path("../../../../scripts/run-ios-simulator-test.sh").getPath(b),
        });
        run_tests.addArtifactArg(tests);
        return run_tests;
    }

    return b.addRunArtifact(tests);
}

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const render_backend = maplibre_build.renderBackend(b);
    const cmake_artifact_dir = maplibre_build.cmakeArtifactDir(b);
    const options = BuildOptions{
        .target = target,
        .optimize = testOptimize(target, b.standardOptimizeOption(.{})),
        .cmake_artifact_dir = cmake_artifact_dir,
        .include_dirs = maplibre_build.includeDirs(b),
        .dependency_library_dirs = maplibre_build.dependencyLibraryDirs(b),
        .render_backend = render_backend,
    };

    const c_tests = addCTests(b, options);

    const run_c_tests = addTestRunStep(b, c_tests, options.target);
    const test_step = b.step("test", "Run Zig C ABI tests");
    test_step.dependOn(&run_c_tests.step);
}
