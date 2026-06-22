const builtin = @import("builtin");
const std = @import("std");
const zigglgen = @import("zigglgen");

const BuildOptions = struct {
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
    include_dirs: []const std.Build.LazyPath,
    dependency_library_dirs: []const std.Build.LazyPath,
    render_backend: RenderBackend,
};

pub const RenderBackend = enum {
    metal,
    opengl,
    vulkan,
};

const NativeArtifactConfig = struct {
    render_backend: []const u8 = "",
    library_path: []const u8 = "",
    import_library_path: []const u8 = "",
    include_dirs: []const []const u8 = &.{},
    library_dirs: []const []const u8 = &.{},
    link_dirs: []const []const u8 = &.{},
    runtime_library_dirs: []const []const u8 = &.{},
    link_libraries: []const []const u8 = &.{},
    frameworks: []const []const u8 = &.{},
};

const NativeArtifactConfigCache = struct {
    path: ?std.Build.LazyPath = null,
    path_loaded: bool = false,
    config: ?NativeArtifactConfig = null,
    config_loaded: bool = false,
};

var native_artifact_config_caches = std.AutoArrayHashMapUnmanaged(*std.Build, NativeArtifactConfigCache){};

fn nativeArtifactConfigCache(b: *std.Build) *NativeArtifactConfigCache {
    const result = native_artifact_config_caches.getOrPut(b.allocator, b) catch @panic("out of memory");
    if (!result.found_existing) {
        result.value_ptr.* = .{};
    }
    return result.value_ptr;
}

fn parseRenderBackend(value: []const u8) RenderBackend {
    return std.meta.stringToEnum(RenderBackend, value) orelse
        std.debug.panic("unsupported render backend in native artifact config: {s}", .{value});
}

fn usesStaticMonolithicLink(config: NativeArtifactConfig) bool {
    return std.ascii.endsWithIgnoreCase(config.library_path, ".a");
}

fn maybeNativeArtifactConfigPath(b: *std.Build) ?std.Build.LazyPath {
    const cache = nativeArtifactConfigCache(b);
    if (!cache.path_loaded) {
        cache.path = b.option(
            std.Build.LazyPath,
            "native-config",
            "Generated native artifact config from the CMake build directory",
        );
        cache.path_loaded = true;
    }
    return cache.path;
}

pub fn nativeArtifactConfigPath(b: *std.Build) std.Build.LazyPath {
    return maybeNativeArtifactConfigPath(b) orelse @panic("missing required -Dnative-config=<path-to-maplibre-native-c.zig-config>");
}

fn pathListSeparator() u8 {
    return if (builtin.os.tag == .windows) ';' else ':';
}

fn parsePathList(b: *std.Build, value: []const u8) []const []const u8 {
    if (value.len == 0) return &.{};

    var count: usize = 0;
    var counter = std.mem.tokenizeScalar(u8, value, pathListSeparator());
    while (counter.next() != null) count += 1;

    const paths = b.allocator.alloc([]const u8, count) catch @panic("out of memory");
    var index: usize = 0;
    var tokens = std.mem.tokenizeScalar(u8, value, pathListSeparator());
    while (tokens.next()) |path| {
        paths[index] = path;
        index += 1;
    }
    return paths;
}

fn parseNativeArtifactConfig(b: *std.Build, config_path: std.Build.LazyPath) NativeArtifactConfig {
    const config_bytes = std.Io.Dir.cwd().readFileAlloc(
        b.graph.io,
        config_path.getPath(b),
        b.allocator,
        .limited(1024 * 1024),
    ) catch |err| std.debug.panic("failed to read native artifact config: {s}: {}", .{ config_path.getPath(b), err });

    var config = NativeArtifactConfig{};
    var lines = std.mem.splitScalar(u8, config_bytes, '\n');
    while (lines.next()) |raw_line| {
        const line = std.mem.trim(u8, raw_line, " \t\r");
        if (line.len == 0) continue;
        const separator = std.mem.indexOfScalar(u8, line, '=') orelse
            std.debug.panic("invalid native artifact config line: {s}", .{line});
        const key = line[0..separator];
        const value = line[separator + 1 ..];

        if (std.mem.eql(u8, key, "render_backend")) {
            config.render_backend = value;
        } else if (std.mem.eql(u8, key, "library_path")) {
            config.library_path = value;
        } else if (std.mem.eql(u8, key, "import_library_path")) {
            config.import_library_path = value;
        } else if (std.mem.eql(u8, key, "include_dirs")) {
            config.include_dirs = parsePathList(b, value);
        } else if (std.mem.eql(u8, key, "library_dirs")) {
            config.library_dirs = parsePathList(b, value);
        } else if (std.mem.eql(u8, key, "link_dirs")) {
            config.link_dirs = parsePathList(b, value);
        } else if (std.mem.eql(u8, key, "runtime_library_dirs")) {
            config.runtime_library_dirs = parsePathList(b, value);
        } else if (std.mem.eql(u8, key, "link_libraries")) {
            config.link_libraries = parsePathList(b, value);
        } else if (std.mem.eql(u8, key, "frameworks")) {
            config.frameworks = parsePathList(b, value);
        }
    }

    if (config.render_backend.len == 0 or config.library_path.len == 0) {
        std.debug.panic("native artifact config is incomplete: {s}", .{config_path.getPath(b)});
    }
    return config;
}

fn maybeNativeArtifactConfig(b: *std.Build) ?NativeArtifactConfig {
    const cache = nativeArtifactConfigCache(b);
    if (cache.config_loaded) return cache.config;
    const config_path = maybeNativeArtifactConfigPath(b) orelse return null;
    cache.config = parseNativeArtifactConfig(b, config_path);
    cache.config_loaded = true;
    return cache.config;
}

fn nativeArtifactConfig(b: *std.Build) NativeArtifactConfig {
    return maybeNativeArtifactConfig(b) orelse @panic("missing required -Dnative-config=<path-to-maplibre-native-c.zig-config>");
}

fn lazyPath(path: []const u8) std.Build.LazyPath {
    return .{ .cwd_relative = path };
}

fn lazyPathsFromStrings(b: *std.Build, paths: []const []const u8) []const std.Build.LazyPath {
    const lazy_paths = b.allocator.alloc(std.Build.LazyPath, paths.len) catch @panic("out of memory");
    for (paths, lazy_paths) |path, *lazy_path_| {
        lazy_path_.* = lazyPath(path);
    }
    return lazy_paths;
}

pub const LinkOptions = struct {
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
    include_dirs: []const std.Build.LazyPath,
};

pub const DependencyOptions = struct {
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
    native_config_path: std.Build.LazyPath,
};

pub const RenderBackendLinkOptions = struct {
    target: std.Build.ResolvedTarget,
    render_backend: RenderBackend,
    dependency_library_dirs: []const std.Build.LazyPath = &.{},
};

pub fn renderBackend(b: *std.Build) RenderBackend {
    return parseRenderBackend(nativeArtifactConfig(b).render_backend);
}

pub fn includeDirs(b: *std.Build) []const std.Build.LazyPath {
    return lazyPathsFromStrings(b, nativeArtifactConfig(b).include_dirs);
}

pub fn addIncludePaths(module: *std.Build.Module, include_dirs: []const std.Build.LazyPath) void {
    for (include_dirs) |include_dir| {
        module.addIncludePath(include_dir);
    }
}

fn addTranslateCIncludePaths(translate_c: *std.Build.Step.TranslateC, include_dirs: []const std.Build.LazyPath) void {
    for (include_dirs) |include_dir| {
        translate_c.addIncludePath(include_dir);
    }
}

pub const CMacro = struct {
    name: []const u8,
    value: ?[]const u8 = null,
};

pub fn sdlTranslateCMacros(target: std.Build.ResolvedTarget) []const CMacro {
    if (target.result.os.tag != .windows or target.result.abi != .msvc) return &.{};
    if (target.result.cpu.arch == .aarch64) {
        return &.{
            .{ .name = "SIZE_MAX", .value = "((size_t)-1)" },
            // Zig translate-c does not define __clang__, so Windows ARM64 UCRT
            // wchar.h selects NEON intrinsics without importing arm_neon.h.
            .{ .name = "_M_CEE", .value = "1" },
            .{ .name = "__clrcall", .value = "__cdecl" },
            .{ .name = "SDL_SINT64_C(c)", .value = "c##LL" },
            .{ .name = "SDL_UINT64_C(c)", .value = "c##ULL" },
        };
    }
    return &.{
        .{ .name = "SIZE_MAX", .value = "((size_t)-1)" },
        .{ .name = "SDL_SINT64_C(c)", .value = "c##LL" },
        .{ .name = "SDL_UINT64_C(c)", .value = "c##ULL" },
    };
}

pub const TranslateCModuleOptions = struct {
    root_source_file: std.Build.LazyPath,
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
    include_dirs: []const std.Build.LazyPath,
    c_macros: []const CMacro = &.{},
};

pub fn translateCModule(b: *std.Build, options: TranslateCModuleOptions) *std.Build.Module {
    const translate_c = b.addTranslateC(.{
        .root_source_file = options.root_source_file,
        .target = options.target,
        .optimize = options.optimize,
    });
    addTranslateCIncludePaths(translate_c, options.include_dirs);
    addPlatformSystemHeaderPaths(b, translate_c, options.target);
    for (options.c_macros) |c_macro| {
        translate_c.defineCMacro(c_macro.name, c_macro.value);
    }
    return translate_c.createModule();
}

fn maplibreNativeCHeader(b: *std.Build) std.Build.LazyPath {
    const header = b.addWriteFiles();
    return header.add("maplibre_native_c_import.h", "#include <maplibre_native_c.h>\n");
}

fn vulkanBindingsHeader(b: *std.Build) std.Build.LazyPath {
    const header = b.addWriteFiles();
    return header.add("vulkan_bindings.h", "#include <vulkan/vulkan.h>\n");
}

fn eglBindingsHeader(b: *std.Build) std.Build.LazyPath {
    const header = b.addWriteFiles();
    return header.add("egl_bindings.h",
        \\#define EGL_EGLEXT_PROTOTYPES 1
        \\#include <EGL/egl.h>
        \\#include <EGL/eglext.h>
        \\
    );
}

pub const RenderBackendTranslateCOptions = struct {
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
    include_dirs: []const std.Build.LazyPath,
    render_backend: RenderBackend,
};

pub fn addRenderBackendTranslateC(b: *std.Build, module: *std.Build.Module, options: RenderBackendTranslateCOptions) void {
    if (options.render_backend == .vulkan) {
        module.addImport("vulkan", translateCModule(b, .{
            .root_source_file = vulkanBindingsHeader(b),
            .target = options.target,
            .optimize = options.optimize,
            .include_dirs = options.include_dirs,
        }));
    }
    if (options.render_backend == .opengl and (options.target.result.os.tag == .linux or options.target.result.os.tag == .macos)) {
        module.addImport("egl", translateCModule(b, .{
            .root_source_file = eglBindingsHeader(b),
            .target = options.target,
            .optimize = options.optimize,
            .include_dirs = options.include_dirs,
        }));
    }
}

pub fn dependencyLibraryDirs(b: *std.Build) []const std.Build.LazyPath {
    return lazyPathsFromStrings(b, nativeArtifactConfig(b).library_dirs);
}

fn addDependencyLibraryPaths(module: *std.Build.Module, dependency_library_dirs: []const std.Build.LazyPath) void {
    for (dependency_library_dirs) |dependency_library_dir| {
        module.addLibraryPath(dependency_library_dir);
        module.addRPath(dependency_library_dir);
    }
}

fn addPlatformSystemHeaderPaths(b: *std.Build, destination: anytype, target: std.Build.ResolvedTarget) void {
    if (!target.result.os.tag.isDarwin() and target.result.os.tag != .linux) return;
    const system_root = b.graph.environ_map.get("MLN_FFI_SYSTEM_ROOT") orelse return;
    if (system_root.len == 0) return;

    if (target.result.os.tag.isDarwin()) {
        destination.addSystemFrameworkPath(.{ .cwd_relative = b.pathJoin(&.{ system_root, "System", "Library", "Frameworks" }) });
    }
    destination.addSystemIncludePath(.{ .cwd_relative = b.pathJoin(&.{ system_root, "usr", "include" }) });
}

pub fn addPlatformSystemPaths(b: *std.Build, module: *std.Build.Module, target: std.Build.ResolvedTarget) void {
    addPlatformSystemHeaderPaths(b, module, target);
    if (!target.result.os.tag.isDarwin() and target.result.os.tag != .linux) return;
    const system_root = b.graph.environ_map.get("MLN_FFI_SYSTEM_ROOT") orelse return;
    if (system_root.len == 0) return;

    module.addLibraryPath(.{ .cwd_relative = b.pathJoin(&.{ system_root, "usr", "lib" }) });
    if (target.result.os.tag == .linux) {
        module.addLibraryPath(.{ .cwd_relative = b.pathJoin(&.{ system_root, "usr", "lib64" }) });
        module.addLibraryPath(.{ .cwd_relative = b.pathJoin(&.{ system_root, "lib" }) });
        module.addLibraryPath(.{ .cwd_relative = b.pathJoin(&.{ system_root, "lib64" }) });
    }
}

fn linkSystemLibraries(module: *std.Build.Module, libraries: []const []const u8) void {
    for (libraries) |library| {
        module.linkSystemLibrary(library, .{});
    }
}

fn linkFrameworks(module: *std.Build.Module, frameworks: []const []const u8) void {
    for (frameworks) |framework| {
        module.linkFramework(framework, .{});
    }
}

fn addLibraryPaths(module: *std.Build.Module, library_dirs: []const std.Build.LazyPath) void {
    for (library_dirs) |library_dir| {
        module.addLibraryPath(library_dir);
    }
}

fn addRPaths(module: *std.Build.Module, library_dirs: []const std.Build.LazyPath) void {
    for (library_dirs) |library_dir| {
        module.addRPath(library_dir);
    }
}

pub fn vulkanLibraryName(target: std.Build.ResolvedTarget) []const u8 {
    return switch (target.result.os.tag) {
        .windows => "vulkan-1",
        else => "vulkan",
    };
}

pub fn isIosSimulator(target: std.Build.ResolvedTarget) bool {
    return target.result.os.tag == .ios and target.result.abi == .simulator;
}

pub fn isIos(target: std.Build.ResolvedTarget) bool {
    return target.result.os.tag == .ios;
}

pub fn testOptimize(target: std.Build.ResolvedTarget, optimize: std.builtin.OptimizeMode) std.builtin.OptimizeMode {
    // Zig Debug iOS tests hit Mach-O/debug-info linker limits in this dependency graph.
    if (isIos(target) and optimize == .Debug) return .ReleaseSafe;
    return optimize;
}

pub fn addRenderBackendOptions(b: *std.Build, module: *std.Build.Module, backend: RenderBackend) void {
    const build_options = b.addOptions();
    build_options.addOption(bool, "supports_metal", backend == .metal);
    build_options.addOption(bool, "supports_opengl", backend == .opengl);
    build_options.addOption(bool, "supports_vulkan", backend == .vulkan);
    module.addOptions("build_options", build_options);
}

pub fn linkRenderBackend(b: *std.Build, module: *std.Build.Module, options: RenderBackendLinkOptions) void {
    addPlatformSystemPaths(b, module, options.target);

    switch (options.render_backend) {
        .metal => {
            module.linkFramework("Metal", .{});
            module.linkFramework("QuartzCore", .{});
        },
        .opengl => switch (options.target.result.os.tag) {
            .linux => {
                module.linkSystemLibrary("EGL", .{});
                module.linkSystemLibrary("GLESv2", .{});
            },
            .macos => {
                if (options.dependency_library_dirs.len == 0) {
                    @panic("macOS OpenGL builds require native artifact config with a library_dirs entry containing EGL and GLESv2");
                }
                addDependencyLibraryPaths(module, options.dependency_library_dirs);
                module.linkSystemLibrary("EGL", .{});
                module.linkSystemLibrary("GLESv2", .{});
            },
            .windows => module.linkSystemLibrary("opengl32", .{}),
            else => unreachable,
        },
        .vulkan => {
            addDependencyLibraryPaths(module, options.dependency_library_dirs);
            module.linkSystemLibrary(vulkanLibraryName(options.target), .{});
        },
    }
}

fn dependencyArgs(options: DependencyOptions) struct {
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
    @"native-config": std.Build.LazyPath,
} {
    return .{
        .target = options.target,
        .optimize = options.optimize,
        .@"native-config" = options.native_config_path,
    };
}

pub fn dependency(b: *std.Build, options: DependencyOptions) *std.Build.Dependency {
    return b.dependencyFromBuildZig(@This(), dependencyArgs(options));
}

pub fn maplibreNativeModule(b: *std.Build, options: DependencyOptions) *std.Build.Module {
    return dependency(b, options).module("maplibre_native");
}

fn repoLinkOptions(options: BuildOptions) LinkOptions {
    return .{
        .target = options.target,
        .optimize = options.optimize,
        .include_dirs = options.include_dirs,
    };
}

pub const IncludeOptions = struct {
    include_dirs: []const std.Build.LazyPath,
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
};

/// Configures the raw C declarations without linking maplibre-native-c.
pub fn addMaplibreNativeIncludes(b: *std.Build, module_: *std.Build.Module, options: IncludeOptions) void {
    addIncludePaths(module_, options.include_dirs);
    module_.link_libc = true;
    module_.addImport("maplibre_native_c", translateCModule(b, .{
        .root_source_file = maplibreNativeCHeader(b),
        .target = options.target,
        .optimize = options.optimize,
        .include_dirs = options.include_dirs,
    }));
}

/// Links a Zig module to the MapLibre Native C library and its backend-specific dependencies.
///
/// Callers provide all filesystem paths explicitly so the helper works both from this
/// package and from external consumers with a different build root layout.
pub fn linkMaplibreNativeC(b: *std.Build, module_: *std.Build.Module, options: LinkOptions) void {
    addMaplibreNativeIncludes(b, module_, .{
        .include_dirs = options.include_dirs,
        .target = options.target,
        .optimize = options.optimize,
    });
    const config = nativeArtifactConfig(b);
    const dependency_library_dirs = lazyPathsFromStrings(b, config.library_dirs);
    const link_dirs = lazyPathsFromStrings(b, config.link_dirs);
    const runtime_library_dirs = lazyPathsFromStrings(b, config.runtime_library_dirs);
    if (usesStaticMonolithicLink(config)) {
        addLibraryPaths(module_, link_dirs);
        linkSystemLibraries(module_, config.link_libraries);
        if (options.target.result.os.tag == .ios) {
            const system_root = b.graph.environ_map.get("MLN_FFI_SYSTEM_ROOT").?;
            module_.addObjectFile(.{ .cwd_relative = b.pathJoin(&.{ system_root, "usr", "lib", "libc++.tbd" }) });
        }
        linkFrameworks(module_, config.frameworks);
    } else if (options.target.result.os.tag == .windows) {
        module_.addObjectFile(lazyPath(config.import_library_path));
    } else {
        addLibraryPaths(module_, link_dirs);
        addRPaths(module_, runtime_library_dirs);
        module_.linkSystemLibrary("maplibre-native-c", .{});
    }
    linkRenderBackend(b, module_, .{
        .target = options.target,
        .render_backend = parseRenderBackend(config.render_backend),
        .dependency_library_dirs = dependency_library_dirs,
    });
}

fn addMaplibreNativeModule(b: *std.Build, options: BuildOptions) *std.Build.Module {
    const maplibre_native = b.addModule("maplibre_native", .{
        .root_source_file = b.path("src/maplibre_native.zig"),
        .target = options.target,
        .optimize = options.optimize,
    });
    linkMaplibreNativeC(b, maplibre_native, repoLinkOptions(options));
    return maplibre_native;
}

fn defaultDocIncludeDirs(b: *std.Build) []const std.Build.LazyPath {
    return &.{b.path("../../include")};
}

fn addMaplibreNativeDocs(
    b: *std.Build,
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
    include_dirs: []const std.Build.LazyPath,
) void {
    const docs_module = b.createModule(.{
        .root_source_file = b.path("src/maplibre_native.zig"),
        .target = target,
        .optimize = optimize,
    });
    addMaplibreNativeIncludes(b, docs_module, .{
        .include_dirs = include_dirs,
        .target = target,
        .optimize = optimize,
    });

    const doc_compile = b.addObject(.{
        .name = "maplibre_native_docs",
        .root_module = docs_module,
    });
    const install_docs = b.addInstallDirectory(.{
        .source_dir = doc_compile.getEmittedDocs(),
        .install_dir = .prefix,
        .install_subdir = "",
    });
    const docs_step = b.step("docs", "Install package documentation into the prefix");
    docs_step.dependOn(&install_docs.step);
}

fn addTestCompile(b: *std.Build, options: BuildOptions, root_source_file: std.Build.LazyPath) *std.Build.Step.Compile {
    const tests = b.addTest(.{
        .root_module = b.createModule(.{
            .root_source_file = root_source_file,
            .target = options.target,
            .optimize = options.optimize,
        }),
        .use_lld = if (isIos(options.target)) false else null,
    });
    if (isIos(options.target)) {
        tests.root_module.addCSourceFile(.{ .file = b.path("../../src/zig_test_support/ios_simulator_dyld_stub.m") });
    }
    linkMaplibreNativeC(b, tests.root_module, repoLinkOptions(options));
    return tests;
}

fn addBindingTests(b: *std.Build, options: BuildOptions, maplibre_native: *std.Build.Module) *std.Build.Step.Compile {
    const tests = addTestCompile(b, options, b.path("tests/main.zig"));
    tests.root_module.addImport("maplibre_native", maplibre_native);
    addRenderBackendOptions(b, tests.root_module, options.render_backend);
    addRenderBackendTranslateC(b, tests.root_module, .{
        .target = options.target,
        .optimize = options.optimize,
        .include_dirs = options.include_dirs,
        .render_backend = options.render_backend,
    });
    if (options.render_backend == .opengl) {
        const gl_bindings = zigglgen.generateBindingsModule(b, if (options.target.result.os.tag == .linux or options.target.result.os.tag == .macos)
            .{ .api = .gles, .version = .@"3.0" }
        else
            .{ .api = .gl, .version = .@"3.0" });
        tests.root_module.addImport("gl", gl_bindings);
        if (options.target.result.os.tag == .windows) {
            const wgl_test_context = b.createModule(.{
                .root_source_file = b.path("../../src/zig_test_support/wgl_context.zig"),
                .target = options.target,
                .optimize = options.optimize,
            });
            wgl_test_context.addImport("gl", gl_bindings);
            tests.root_module.addImport("wgl_test_context", wgl_test_context);
        }
    }
    if (options.render_backend == .metal) {
        if (options.target.result.os.tag == .ios) {
            tests.root_module.addCSourceFile(.{ .file = b.path("tests/metal_support_ios.m") });
            tests.root_module.linkSystemLibrary("objc", .{});
            tests.root_module.linkFramework("Foundation", .{});
        }
        if (options.target.result.os.tag == .macos) {
            tests.root_module.addCSourceFile(.{ .file = b.path("tests/metal_support_macos.m") });
            tests.root_module.linkFramework("AppKit", .{});
        }
    }
    return tests;
}

pub fn addTestRunStep(
    b: *std.Build,
    tests: *std.Build.Step.Compile,
    target: std.Build.ResolvedTarget,
    simulator_runner: std.Build.LazyPath,
) *std.Build.Step.Run {
    if (isIosSimulator(target)) {
        const run_tests = b.addSystemCommand(&.{
            "bash",
            simulator_runner.getPath(b),
        });
        run_tests.addArtifactArg(tests);
        return run_tests;
    }

    return b.addRunArtifact(tests);
}

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const include_dirs_from_cli = b.option(
        []const std.Build.LazyPath,
        "include-dir",
        "Include directory. Repeat for project, dependency, and backend headers.",
    );

    addMaplibreNativeDocs(
        b,
        target,
        optimize,
        include_dirs_from_cli orelse defaultDocIncludeDirs(b),
    );

    _ = maybeNativeArtifactConfigPath(b) orelse return;

    const backend = renderBackend(b);
    const options = BuildOptions{
        .target = target,
        .optimize = testOptimize(target, optimize),
        .include_dirs = includeDirs(b),
        .dependency_library_dirs = dependencyLibraryDirs(b),
        .render_backend = backend,
    };

    const maplibre_native = addMaplibreNativeModule(b, options);

    const test_sources = [_]std.Build.LazyPath{
        b.path("src/status.zig"),
        b.path("src/runtime.zig"),
        b.path("src/logging.zig"),
        b.path("src/map.zig"),
    };

    const test_step = b.step("test", "Run Zig binding tests");

    const binding_tests = addBindingTests(b, options, maplibre_native);
    b.default_step.dependOn(&binding_tests.step);
    const run_binding_tests = addTestRunStep(b, binding_tests, options.target, b.path("../../scripts/run-ios-simulator-test.sh"));
    test_step.dependOn(&run_binding_tests.step);

    for (test_sources) |source| {
        const tests = addTestCompile(b, options, source);
        b.default_step.dependOn(&tests.step);
        const run_tests = addTestRunStep(b, tests, options.target, b.path("../../scripts/run-ios-simulator-test.sh"));
        test_step.dependOn(&run_tests.step);
    }
}
