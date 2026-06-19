const std = @import("std");
const zigglgen = @import("zigglgen");

const BuildOptions = struct {
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
    cmake_artifact_dir: std.Build.LazyPath,
    include_dirs: []const std.Build.LazyPath,
    dependency_library_dirs: []const std.Build.LazyPath,
    render_backend: RenderBackend,
};

pub const RenderBackend = enum {
    metal,
    opengl,
    vulkan,
};

const ArtifactShape = enum {
    shared_private,
    static_monolithic,
};

const NativeMetadataJson = struct {
    schema_version: u32,
    render_backend: []const u8,
    artifact_shape: []const u8,
    core_library_path: []const u8,
    windows_import_library_path: ?[]const u8 = null,
    public_include_dirs: []const []const u8 = &.{},
    binding_include_dirs: []const []const u8 = &.{},
    dependency_library_dirs: []const []const u8 = &.{},
    runtime_search_paths: []const []const u8 = &.{},
    static_library_dirs: []const []const u8 = &.{},
    static_libraries: []const []const u8 = &.{},
    static_system_libraries: []const []const u8 = &.{},
    static_frameworks: []const []const u8 = &.{},
    c_abi_version: u32,
};

var cached_native_metadata_path: ?std.Build.LazyPath = null;
var native_metadata_path_loaded = false;
var cached_native_metadata: ?NativeMetadataJson = null;
var native_metadata_loaded = false;

fn parseRenderBackend(value: []const u8) RenderBackend {
    return std.meta.stringToEnum(RenderBackend, value) orelse
        std.debug.panic("unsupported render backend in native metadata: {s}", .{value});
}

fn parseArtifactShape(value: []const u8) ArtifactShape {
    if (std.mem.eql(u8, value, "shared-private")) return .shared_private;
    if (std.mem.eql(u8, value, "static-monolithic")) return .static_monolithic;
    std.debug.panic("unsupported artifact shape in native metadata: {s}", .{value});
}

fn nativeMetadataPath(b: *std.Build) ?std.Build.LazyPath {
    if (!native_metadata_path_loaded) {
        cached_native_metadata_path = b.option(
            std.Build.LazyPath,
            "native-metadata",
            "Generated native artifact metadata JSON from the CMake build directory",
        );
        native_metadata_path_loaded = true;
    }
    return cached_native_metadata_path;
}

fn nativeMetadata(b: *std.Build) ?NativeMetadataJson {
    if (native_metadata_loaded) return cached_native_metadata;
    const metadata_path = nativeMetadataPath(b) orelse return null;
    const metadata_bytes = std.Io.Dir.cwd().readFileAlloc(
        b.graph.io,
        metadata_path.getPath(b),
        b.allocator,
        .limited(1024 * 1024),
    ) catch |err| std.debug.panic("failed to read native metadata: {s}: {}", .{ metadata_path.getPath(b), err });
    const parsed = std.json.parseFromSlice(
        NativeMetadataJson,
        b.allocator,
        metadata_bytes,
        .{ .ignore_unknown_fields = true },
    ) catch |err| std.debug.panic("failed to parse native metadata: {s}: {}", .{ metadata_path.getPath(b), err });
    if (parsed.value.schema_version != 1) {
        std.debug.panic("unsupported native metadata schema version: {}", .{parsed.value.schema_version});
    }
    if (parsed.value.c_abi_version != 0) {
        std.debug.panic("unsupported native C ABI version: {}", .{parsed.value.c_abi_version});
    }
    cached_native_metadata = parsed.value;
    native_metadata_loaded = true;
    return cached_native_metadata;
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

fn parentDir(path: []const u8) std.Build.LazyPath {
    return lazyPath(std.fs.path.dirname(path) orelse ".");
}

pub const LinkOptions = struct {
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
    cmake_artifact_dir: std.Build.LazyPath,
    render_backend: RenderBackend,
    include_dirs: []const std.Build.LazyPath,
    dependency_library_dirs: []const std.Build.LazyPath = &.{},
};

pub const DependencyOptions = struct {
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
    cmake_artifact_dir: std.Build.LazyPath,
    include_dirs: []const std.Build.LazyPath,
    render_backend: RenderBackend,
    dependency_library_dirs: []const std.Build.LazyPath = &.{},
};

pub const RenderBackendLinkOptions = struct {
    target: std.Build.ResolvedTarget,
    render_backend: RenderBackend,
    dependency_library_dirs: []const std.Build.LazyPath = &.{},
};

pub fn renderBackend(b: *std.Build) RenderBackend {
    const cli_backend = b.option(
        RenderBackend,
        "render-backend",
        "Render backend built into the CMake artifact: metal, opengl, or vulkan",
    );
    if (nativeMetadata(b)) |metadata| return parseRenderBackend(metadata.render_backend);
    return cli_backend orelse @panic("missing required -Drender-backend=metal|opengl|vulkan");
}

pub fn cmakeArtifactDir(b: *std.Build) std.Build.LazyPath {
    const cli_artifact_dir = b.option(
        std.Build.LazyPath,
        "cmake-artifact-dir",
        "Directory containing the CMake-built maplibre-native-c library",
    );
    if (nativeMetadata(b)) |metadata| return parentDir(metadata.core_library_path);
    return cli_artifact_dir orelse @panic("missing required -Dcmake-artifact-dir=<path-to-cmake-artifacts>");
}

pub fn includeDirs(b: *std.Build) []const std.Build.LazyPath {
    const cli_include_dirs = b.option(
        []const std.Build.LazyPath,
        "include-dir",
        "Include directory. Repeat for project, dependency, and backend headers.",
    );
    if (nativeMetadata(b)) |metadata| {
        if (metadata.binding_include_dirs.len != 0) return lazyPathsFromStrings(b, metadata.binding_include_dirs);
        return lazyPathsFromStrings(b, metadata.public_include_dirs);
    }
    return cli_include_dirs orelse @panic("missing required -Dinclude-dir=<path>; repeat for additional include roots");
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
    const cli_dependency_library_dirs = b.option(
        []const std.Build.LazyPath,
        "dependency-library-dir",
        "Dependency library directory. Repeat for backend runtime libraries.",
    );
    if (nativeMetadata(b)) |metadata| return lazyPathsFromStrings(b, metadata.dependency_library_dirs);
    return cli_dependency_library_dirs orelse &.{};
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

pub fn vulkanLibraryName(target: std.Build.ResolvedTarget) []const u8 {
    return switch (target.result.os.tag) {
        .windows => "vulkan-1",
        else => "vulkan",
    };
}

pub fn isIosSimulator(target: std.Build.ResolvedTarget) bool {
    return target.result.os.tag == .ios and target.result.abi == .simulator;
}

pub fn testOptimize(target: std.Build.ResolvedTarget, optimize: std.builtin.OptimizeMode) std.builtin.OptimizeMode {
    // Zig Debug iOS simulator tests hit Mach-O/debug-info linker limits in this dependency graph.
    if (isIosSimulator(target) and optimize == .Debug) return .ReleaseSafe;
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
                    @panic("macOS OpenGL builds require -Ddependency-library-dir=<path> containing EGL and GLESv2");
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
    @"cmake-artifact-dir": std.Build.LazyPath,
    @"include-dir": []const std.Build.LazyPath,
    @"render-backend": RenderBackend,
    @"dependency-library-dir": []const std.Build.LazyPath,
} {
    return .{
        .target = options.target,
        .optimize = options.optimize,
        .@"cmake-artifact-dir" = options.cmake_artifact_dir,
        .@"include-dir" = options.include_dirs,
        .@"render-backend" = options.render_backend,
        .@"dependency-library-dir" = options.dependency_library_dirs,
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
        .cmake_artifact_dir = options.cmake_artifact_dir,
        .render_backend = options.render_backend,
        .include_dirs = options.include_dirs,
        .dependency_library_dirs = options.dependency_library_dirs,
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
    if (nativeMetadata(b)) |metadata| {
        const dependency_library_dirs = lazyPathsFromStrings(b, metadata.dependency_library_dirs);
        switch (parseArtifactShape(metadata.artifact_shape)) {
            .shared_private => {
                if (options.target.result.os.tag == .windows) {
                    module_.addObjectFile(lazyPath(metadata.windows_import_library_path orelse metadata.core_library_path));
                } else {
                    module_.addLibraryPath(parentDir(metadata.core_library_path));
                    module_.addRPath(parentDir(metadata.core_library_path));
                    for (metadata.runtime_search_paths) |runtime_search_path| {
                        module_.addRPath(lazyPath(runtime_search_path));
                    }
                    module_.linkSystemLibrary("maplibre-native-c", .{});
                }
            },
            .static_monolithic => {
                addLibraryPaths(module_, lazyPathsFromStrings(b, metadata.static_library_dirs));
                if (metadata.static_libraries.len == 0) {
                    module_.addLibraryPath(parentDir(metadata.core_library_path));
                    module_.linkSystemLibrary("maplibre-native-c", .{});
                } else {
                    linkSystemLibraries(module_, metadata.static_libraries);
                }
                if (options.target.result.os.tag == .ios) {
                    if (b.graph.environ_map.get("MLN_FFI_SYSTEM_ROOT")) |system_root| {
                        if (system_root.len != 0) {
                            module_.addObjectFile(.{ .cwd_relative = b.pathJoin(&.{ system_root, "usr", "lib", "libc++.tbd" }) });
                        }
                    }
                }
                linkSystemLibraries(module_, metadata.static_system_libraries);
                linkFrameworks(module_, metadata.static_frameworks);
            },
        }
        linkRenderBackend(b, module_, .{
            .target = options.target,
            .render_backend = parseRenderBackend(metadata.render_backend),
            .dependency_library_dirs = dependency_library_dirs,
        });
        return;
    }
    if (options.target.result.os.tag == .windows) {
        module_.addObjectFile(options.cmake_artifact_dir.path(b, "maplibre-native-c.lib"));
    } else if (isIosSimulator(options.target)) {
        module_.addLibraryPath(options.cmake_artifact_dir);
        module_.addRPath(options.cmake_artifact_dir);
        module_.linkSystemLibrary("maplibre-native-c", .{});
    } else if (options.target.result.os.tag == .ios) {
        module_.addLibraryPath(options.cmake_artifact_dir);
        module_.addLibraryPath(options.cmake_artifact_dir.path(b, "maplibre-native"));
        module_.addLibraryPath(options.cmake_artifact_dir.path(b, "maplibre-native/vendor/maplibre-tile-spec/cpp"));
        module_.linkSystemLibrary("maplibre-native-c", .{});
        module_.linkSystemLibrary("mbgl-core", .{});
        module_.linkSystemLibrary("mbgl-freetype", .{});
        module_.linkSystemLibrary("mbgl-harfbuzz", .{});
        module_.linkSystemLibrary("mbgl-vendor-csscolorparser", .{});
        module_.linkSystemLibrary("mbgl-vendor-nunicode", .{});
        module_.linkSystemLibrary("mbgl-vendor-parsedate", .{});
        module_.linkSystemLibrary("mbgl-vendor-sqlite", .{});
        module_.linkSystemLibrary("mlt-cpp", .{});
        if (b.graph.environ_map.get("MLN_FFI_SYSTEM_ROOT")) |system_root| {
            if (system_root.len != 0) {
                module_.addObjectFile(.{ .cwd_relative = b.pathJoin(&.{ system_root, "usr", "lib", "libc++.tbd" }) });
            }
        }
        module_.linkSystemLibrary("objc", .{});
        module_.linkSystemLibrary("sqlite3", .{});
        module_.linkSystemLibrary("z", .{});
        module_.linkFramework("CoreFoundation", .{});
        module_.linkFramework("CoreGraphics", .{});
        module_.linkFramework("CoreText", .{});
        module_.linkFramework("Foundation", .{});
        module_.linkFramework("ImageIO", .{});
        module_.linkFramework("MetalKit", .{});
    } else {
        module_.addLibraryPath(options.cmake_artifact_dir);
        module_.addRPath(options.cmake_artifact_dir);
        module_.linkSystemLibrary("maplibre-native-c", .{});
    }
    linkRenderBackend(b, module_, .{
        .target = options.target,
        .render_backend = options.render_backend,
        .dependency_library_dirs = options.dependency_library_dirs,
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
        .use_lld = if (isIosSimulator(options.target)) false else null,
    });
    if (isIosSimulator(options.target)) {
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
        if (isIosSimulator(options.target)) {
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

    const cmake_artifact_dir = b.option(
        std.Build.LazyPath,
        "cmake-artifact-dir",
        "Directory containing the CMake-built maplibre-native-c library",
    ) orelse return;

    const include_dirs = include_dirs_from_cli orelse
        @panic("missing required -Dinclude-dir=<path>; repeat for additional include roots");

    const backend = renderBackend(b);
    const options = BuildOptions{
        .target = target,
        .optimize = testOptimize(target, optimize),
        .cmake_artifact_dir = cmake_artifact_dir,
        .include_dirs = include_dirs,
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
