// This manifest sits at the repository root rather than beside the sources it
// describes, because Zig resolves a package from the repository root. Source
// and test paths therefore name their locations under bindings/zig/ explicitly.

const builtin = @import("builtin");
const std = @import("std");
const zigglgen = @import("zigglgen");

const BuildOptions = struct {
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
    native_install_dir: std.Build.LazyPath,
    include_dirs: []const std.Build.LazyPath,
    dependency_library_dirs: []const std.Build.LazyPath,
    render_backend: RenderBackend,
    system_root: ?std.Build.LazyPath,
    target_libc: ?std.Build.LazyPath,
};

pub const RenderBackend = enum {
    metal,
    opengl,
    vulkan,
};

const ArtifactDescriptor = struct {
    renderBackend: []const u8,
    zigTarget: []const u8,
};

fn parseRenderBackend(value: []const u8) RenderBackend {
    return std.meta.stringToEnum(RenderBackend, value) orelse
        std.debug.panic("unsupported render backend: {s}", .{value});
}

fn lazyPath(path: []const u8) std.Build.LazyPath {
    return .{ .cwd_relative = path };
}

fn installPath(b: *std.Build, install_dir: std.Build.LazyPath, sub_path: []const u8) std.Build.LazyPath {
    return lazyPath(b.pathJoin(&.{ install_dir.getPath(b), sub_path }));
}

fn installedArtifactDescriptor(b: *std.Build, install_dir: std.Build.LazyPath) ArtifactDescriptor {
    const descriptor_path = b.pathJoin(&.{ install_dir.getPath(b), "share", "maplibre-native-c", "artifact.json" });
    const descriptor = std.Io.Dir.cwd().readFileAlloc(b.graph.io, descriptor_path, b.allocator, .limited(4096)) catch
        std.debug.panic("failed to read native artifact descriptor: {s}", .{descriptor_path});
    const parsed = std.json.parseFromSlice(
        ArtifactDescriptor,
        b.allocator,
        descriptor,
        .{ .ignore_unknown_fields = true },
    ) catch std.debug.panic("invalid native artifact descriptor: {s}", .{descriptor_path});
    return parsed.value;
}

fn withInstallIncludeDir(b: *std.Build, install_dir: std.Build.LazyPath, dependency_include_dirs: []const std.Build.LazyPath) []const std.Build.LazyPath {
    const include_dirs = b.allocator.alloc(std.Build.LazyPath, dependency_include_dirs.len + 1) catch @panic("out of memory");
    include_dirs[0] = installPath(b, install_dir, "include");
    @memcpy(include_dirs[1..], dependency_include_dirs);
    return include_dirs;
}

fn nativeRuntimeDir(b: *std.Build, install_dir: std.Build.LazyPath, target: std.Build.ResolvedTarget) std.Build.LazyPath {
    return installPath(b, install_dir, if (target.result.os.tag == .windows) "bin" else "lib");
}

fn staticIosLinkLibraries() []const []const u8 {
    return &.{
        "maplibre-native-c",
        "c++",
        "objc",
        "sqlite3",
        "z",
    };
}

fn staticIosFrameworks(render_backend_: RenderBackend) []const []const u8 {
    return if (render_backend_ == .metal)
        &.{ "CoreFoundation", "CoreGraphics", "CoreText", "Foundation", "ImageIO", "Metal", "MetalKit", "QuartzCore" }
    else
        &.{ "CoreFoundation", "CoreGraphics", "CoreText", "Foundation", "ImageIO", "Metal", "MetalKit" };
}

pub const LinkOptions = struct {
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
    include_dirs: []const std.Build.LazyPath,
    native_install_dir: std.Build.LazyPath,
    render_backend: RenderBackend,
    dependency_library_dirs: []const std.Build.LazyPath,
    system_root: ?std.Build.LazyPath,
};

pub const DependencyOptions = struct {
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
    native_install_dir: std.Build.LazyPath,
    render_backend: RenderBackend,
    dependency_include_dirs: []const std.Build.LazyPath = &.{},
    dependency_library_dirs: []const std.Build.LazyPath = &.{},
    system_root: ?std.Build.LazyPath = null,
};

pub const RenderBackendLinkOptions = struct {
    target: std.Build.ResolvedTarget,
    render_backend: RenderBackend,
    dependency_library_dirs: []const std.Build.LazyPath = &.{},
    system_root: ?std.Build.LazyPath = null,
};

pub fn renderBackend(b: *std.Build, install_dir: std.Build.LazyPath) RenderBackend {
    if (b.option([]const u8, "render-backend", "Override the installed native render backend")) |value| {
        return parseRenderBackend(value);
    }

    return parseRenderBackend(installedArtifactDescriptor(b, install_dir).renderBackend);
}

pub fn nativeTarget(b: *std.Build, install_dir: std.Build.LazyPath) std.Build.ResolvedTarget {
    const descriptor = installedArtifactDescriptor(b, install_dir);
    const artifact_query = std.Target.Query.parse(.{ .arch_os_abi = descriptor.zigTarget }) catch
        std.debug.panic("invalid Zig target in native artifact descriptor: {s}", .{descriptor.zigTarget});
    const artifact_target = b.resolveTargetQuery(artifact_query);
    const host_target = b.graph.host.result;
    const default_target: std.Target.Query = if (artifact_target.result.cpu.arch == host_target.cpu.arch and
        artifact_target.result.os.tag == host_target.os.tag and
        artifact_target.result.abi == host_target.abi)
        .{}
    else
        artifact_query;
    return b.standardTargetOptions(.{ .default_target = default_target });
}

pub fn includeDirs(b: *std.Build) []const std.Build.LazyPath {
    return withInstallIncludeDir(b, nativeInstallDirPath(b), dependencyIncludeDirs(b));
}

pub fn installedIncludeDirs(b: *std.Build, install_dir: std.Build.LazyPath, dependency_include_dirs: []const std.Build.LazyPath) []const std.Build.LazyPath {
    return withInstallIncludeDir(b, install_dir, dependency_include_dirs);
}

pub fn nativeInstallDirPath(b: *std.Build) std.Build.LazyPath {
    return b.option(std.Build.LazyPath, "native-install-dir", "CMake install prefix for maplibre-native-c") orelse
        @panic("missing required -Dnative-install-dir=<path-to-cmake-install-prefix>");
}

pub fn maybeNativeInstallDirPath(b: *std.Build) ?std.Build.LazyPath {
    return b.option(std.Build.LazyPath, "native-install-dir", "CMake install prefix for maplibre-native-c");
}

pub fn dependencyIncludeDirs(b: *std.Build) []const std.Build.LazyPath {
    return b.option([]const std.Build.LazyPath, "dependency-include-dir", "Additional local dependency include directory") orelse &.{};
}

pub fn dependencyLibraryDirs(b: *std.Build) []const std.Build.LazyPath {
    return b.option([]const std.Build.LazyPath, "dependency-library-dir", "Additional local dependency library directory") orelse &.{};
}

pub fn maybeSystemRootPath(b: *std.Build) ?std.Build.LazyPath {
    return b.option(std.Build.LazyPath, "system-root", "Target platform SDK or sysroot");
}

fn takeTargetLibCPath(b: *std.Build) ?std.Build.LazyPath {
    const path = b.libc_file orelse return null;
    // `--libc` is a global Zig build option. Keep it off host tools such as
    // zigglgen, and attach it only to target executables.
    b.libc_file = null;
    return lazyPath(path);
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
    system_root: ?std.Build.LazyPath = null,
};

pub fn translateCModule(b: *std.Build, options: TranslateCModuleOptions) *std.Build.Module {
    const translate_c = b.addTranslateC(.{
        .root_source_file = options.root_source_file,
        .target = options.target,
        .optimize = options.optimize,
    });
    addTranslateCIncludePaths(translate_c, options.include_dirs);
    addPlatformSystemHeaderPaths(b, translate_c, options.target, options.system_root);
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
    system_root: ?std.Build.LazyPath = null,
};

pub fn addRenderBackendTranslateC(b: *std.Build, module: *std.Build.Module, options: RenderBackendTranslateCOptions) void {
    if (options.render_backend == .vulkan) {
        module.addImport("vulkan", translateCModule(b, .{
            .root_source_file = vulkanBindingsHeader(b),
            .target = options.target,
            .optimize = options.optimize,
            .include_dirs = options.include_dirs,
            .system_root = options.system_root,
        }));
    }
    if (options.render_backend == .opengl and (options.target.result.os.tag == .linux or options.target.result.os.tag == .macos)) {
        module.addImport("egl", translateCModule(b, .{
            .root_source_file = eglBindingsHeader(b),
            .target = options.target,
            .optimize = options.optimize,
            .include_dirs = options.include_dirs,
            .system_root = options.system_root,
        }));
    }
}

fn addDependencyLibraryPaths(module: *std.Build.Module, dependency_library_dirs: []const std.Build.LazyPath) void {
    for (dependency_library_dirs) |dependency_library_dir| {
        module.addLibraryPath(dependency_library_dir);
        module.addRPath(dependency_library_dir);
    }
}

fn addNativeVulkanSdkPath(b: *std.Build, module: *std.Build.Module, target: std.Build.ResolvedTarget, dependency_library_dirs: []const std.Build.LazyPath) void {
    if (dependency_library_dirs.len != 0) return;
    if (target.result.os.tag != .windows and target.result.os.tag != .macos) return;
    const library_dir = if (target.result.os.tag == .macos)
        b.graph.environ_map.get("MLN_FFI_VULKAN_LOADER_DIR") orelse
            if (b.graph.environ_map.get("VULKAN_SDK")) |sdk| b.pathJoin(&.{ sdk, "lib" }) else return
    else if (b.graph.environ_map.get("VULKAN_SDK")) |sdk|
        b.pathJoin(&.{ sdk, "Lib" })
    else
        @panic("native Windows Vulkan builds require VULKAN_SDK");
    module.addLibraryPath(.{ .cwd_relative = library_dir });
    module.addRPath(.{ .cwd_relative = library_dir });
}

fn addPlatformSystemHeaderPaths(b: *std.Build, destination: anytype, target: std.Build.ResolvedTarget, system_root: ?std.Build.LazyPath) void {
    if (!target.result.os.tag.isDarwin() and target.result.os.tag != .linux) return;
    const root = system_root orelse return;
    const system_root_path = root.getPath(b);

    if (target.result.os.tag.isDarwin()) {
        destination.addSystemFrameworkPath(.{ .cwd_relative = b.pathJoin(&.{ system_root_path, "System", "Library", "Frameworks" }) });
    }
    destination.addSystemIncludePath(.{ .cwd_relative = b.pathJoin(&.{ system_root_path, "usr", "include" }) });
}

pub fn addPlatformSystemPaths(b: *std.Build, module: *std.Build.Module, target: std.Build.ResolvedTarget, system_root: ?std.Build.LazyPath) void {
    addPlatformSystemHeaderPaths(b, module, target, system_root);
    if (!target.result.os.tag.isDarwin() and target.result.os.tag != .linux) return;
    // Android's libraries live under a target- and API-specific directory that
    // the binding tasks pass as a dependency library directory.
    if (target.result.abi == .android) return;
    const root = system_root orelse return;
    const system_root_path = root.getPath(b);

    module.addLibraryPath(.{ .cwd_relative = b.pathJoin(&.{ system_root_path, "usr", "lib" }) });
    if (target.result.os.tag == .linux) {
        module.addLibraryPath(.{ .cwd_relative = b.pathJoin(&.{ system_root_path, "usr", "lib64" }) });
        module.addLibraryPath(.{ .cwd_relative = b.pathJoin(&.{ system_root_path, "lib" }) });
        module.addLibraryPath(.{ .cwd_relative = b.pathJoin(&.{ system_root_path, "lib64" }) });
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

pub fn isAppleMobile(target: std.Build.ResolvedTarget) bool {
    return switch (target.result.os.tag) {
        .ios, .tvos => true,
        else => false,
    };
}

pub fn isAppleSimulator(target: std.Build.ResolvedTarget) bool {
    return isAppleMobile(target) and target.result.abi == .simulator;
}

pub fn testOptimize(target: std.Build.ResolvedTarget, optimize: std.builtin.OptimizeMode) std.builtin.OptimizeMode {
    // Zig Debug Apple-mobile tests hit Mach-O/debug-info linker limits in this dependency graph.
    if (isAppleMobile(target) and optimize == .Debug) return .ReleaseSafe;
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
    addPlatformSystemPaths(b, module, options.target, options.system_root);

    switch (options.render_backend) {
        .metal => {
            module.linkFramework("Metal", .{});
            module.linkFramework("QuartzCore", .{});
        },
        .opengl => switch (options.target.result.os.tag) {
            .linux => {
                addDependencyLibraryPaths(module, options.dependency_library_dirs);
                module.linkSystemLibrary("EGL", .{});
                module.linkSystemLibrary("GLESv2", .{});
            },
            .macos => {
                addDependencyLibraryPaths(module, options.dependency_library_dirs);
                module.linkSystemLibrary("EGL", .{});
                module.linkSystemLibrary("GLESv2", .{});
            },
            .windows => module.linkSystemLibrary("opengl32", .{}),
            else => unreachable,
        },
        .vulkan => {
            addNativeVulkanSdkPath(b, module, options.target, options.dependency_library_dirs);
            addDependencyLibraryPaths(module, options.dependency_library_dirs);
            module.linkSystemLibrary(vulkanLibraryName(options.target), .{});
        },
    }
}

fn dependencyArgs(options: DependencyOptions) struct {
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
    @"native-install-dir": std.Build.LazyPath,
    @"render-backend": []const u8,
    @"dependency-include-dir": []const std.Build.LazyPath,
    @"dependency-library-dir": []const std.Build.LazyPath,
    @"system-root": ?std.Build.LazyPath,
} {
    return .{
        .target = options.target,
        .optimize = options.optimize,
        .@"native-install-dir" = options.native_install_dir,
        .@"render-backend" = @tagName(options.render_backend),
        .@"dependency-include-dir" = options.dependency_include_dirs,
        .@"dependency-library-dir" = options.dependency_library_dirs,
        .@"system-root" = options.system_root,
    };
}

pub fn dependency(b: *std.Build, options: DependencyOptions) *std.Build.Dependency {
    return b.dependencyFromBuildZig(@This(), dependencyArgs(options));
}

pub fn maplibreNativeModule(b: *std.Build, options: DependencyOptions) *std.Build.Module {
    return dependency(b, options).module("maplibre_native_ffi");
}

fn repoLinkOptions(options: BuildOptions) LinkOptions {
    return .{
        .target = options.target,
        .optimize = options.optimize,
        .include_dirs = options.include_dirs,
        .native_install_dir = options.native_install_dir,
        .render_backend = options.render_backend,
        .dependency_library_dirs = options.dependency_library_dirs,
        .system_root = options.system_root,
    };
}

pub const IncludeOptions = struct {
    include_dirs: []const std.Build.LazyPath,
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
    system_root: ?std.Build.LazyPath = null,
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
        .system_root = options.system_root,
    }));
}

/// Links a Zig module to the MapLibre Native C library and its backend-specific dependencies.
///
/// Callers provide all filesystem paths explicitly so the helper works both from this
/// package and from external consumers with a different build root layout.
pub fn linkMaplibreNativeC(b: *std.Build, module_: *std.Build.Module, options: LinkOptions) void {
    if (options.target.result.os.tag == .emscripten) {
        @panic("Emscripten consumers must link the installed archives with emcc");
    }
    addMaplibreNativeIncludes(b, module_, .{
        .include_dirs = options.include_dirs,
        .target = options.target,
        .optimize = options.optimize,
        .system_root = options.system_root,
    });
    const link_dirs = &.{installPath(b, options.native_install_dir, "lib")};
    const runtime_library_dirs = &.{nativeRuntimeDir(b, options.native_install_dir, options.target)};
    if (isAppleMobile(options.target) and !isAppleSimulator(options.target)) {
        addLibraryPaths(module_, link_dirs);
        linkSystemLibraries(module_, staticIosLinkLibraries());
        const system_root = options.system_root orelse
            @panic("iOS and tvOS device builds require -Dsystem-root=<path-to-SDK>");
        module_.addObjectFile(.{ .cwd_relative = b.pathJoin(&.{ system_root.getPath(b), "usr", "lib", "libc++.tbd" }) });
        linkFrameworks(module_, staticIosFrameworks(options.render_backend));
    } else if (options.target.result.os.tag == .windows) {
        module_.addObjectFile(installPath(b, options.native_install_dir, "lib/maplibre-native-c.lib"));
    } else {
        addLibraryPaths(module_, link_dirs);
        addRPaths(module_, runtime_library_dirs);
        module_.linkSystemLibrary("maplibre-native-c", .{ .use_pkg_config = .no });
    }
    linkRenderBackend(b, module_, .{
        .target = options.target,
        .render_backend = options.render_backend,
        .dependency_library_dirs = options.dependency_library_dirs,
        .system_root = options.system_root,
    });
}

fn addMaplibreNativeModule(b: *std.Build, options: BuildOptions) *std.Build.Module {
    const maplibre_native_ffi = b.addModule("maplibre_native_ffi", .{
        .root_source_file = b.path("bindings/zig/src/maplibre_native_ffi.zig"),
        .target = options.target,
        .optimize = options.optimize,
    });
    linkMaplibreNativeC(b, maplibre_native_ffi, repoLinkOptions(options));
    return maplibre_native_ffi;
}

fn defaultDocIncludeDirs(b: *std.Build) []const std.Build.LazyPath {
    return &.{b.path("include")};
}

fn addMaplibreNativeDocs(
    b: *std.Build,
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
    include_dirs: []const std.Build.LazyPath,
) void {
    const docs_module = b.createModule(.{
        .root_source_file = b.path("bindings/zig/src/maplibre_native_ffi.zig"),
        .target = target,
        .optimize = optimize,
    });
    addMaplibreNativeIncludes(b, docs_module, .{
        .include_dirs = include_dirs,
        .target = target,
        .optimize = optimize,
    });

    const doc_compile = b.addObject(.{
        .name = "maplibre_native_ffi_docs",
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
        .use_lld = if (isAppleMobile(options.target)) false else null,
    });
    tests.setLibCFile(options.target_libc);
    if (isAppleMobile(options.target)) {
        tests.root_module.addCSourceFile(.{ .file = b.path("src/zig_test_support/ios_simulator_dyld_stub.m") });
    }
    linkMaplibreNativeC(b, tests.root_module, repoLinkOptions(options));
    return tests;
}

fn addBindingTests(b: *std.Build, options: BuildOptions, maplibre_native_ffi: *std.Build.Module) *std.Build.Step.Compile {
    const tests = addTestCompile(b, options, b.path("bindings/zig/tests/main.zig"));
    tests.root_module.addImport("maplibre_native_ffi", maplibre_native_ffi);
    addRenderBackendOptions(b, tests.root_module, options.render_backend);
    addRenderBackendTranslateC(b, tests.root_module, .{
        .target = options.target,
        .optimize = options.optimize,
        .include_dirs = options.include_dirs,
        .render_backend = options.render_backend,
        .system_root = options.system_root,
    });
    if (options.render_backend == .opengl) {
        const gl_bindings = zigglgen.generateBindingsModule(b, if (options.target.result.os.tag == .linux or options.target.result.os.tag == .macos)
            .{ .api = .gles, .version = .@"3.0" }
        else
            .{ .api = .gl, .version = .@"3.0" });
        tests.root_module.addImport("gl", gl_bindings);
        if (options.target.result.os.tag == .windows) {
            const wgl_test_context = b.createModule(.{
                .root_source_file = b.path("src/zig_test_support/wgl_context.zig"),
                .target = options.target,
                .optimize = options.optimize,
            });
            wgl_test_context.addImport("gl", gl_bindings);
            tests.root_module.addImport("wgl_test_context", wgl_test_context);
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
    if (isAppleSimulator(target)) {
        const run_tests = b.addSystemCommand(&.{
            "bash",
            simulator_runner.getPath(b),
        });
        run_tests.addArtifactArg(tests);
        run_tests.setEnvironmentVariable(
            "MLN_FFI_SIMULATOR_RUNTIME",
            if (target.result.os.tag == .tvos) "tvOS" else "iOS",
        );
        return run_tests;
    }

    return b.addRunArtifact(tests);
}

fn addAndroidTestRunStep(
    b: *std.Build,
    tests: []const *std.Build.Step.Compile,
    target: std.Build.ResolvedTarget,
    native_install_dir: std.Build.LazyPath,
    android_runner: std.Build.LazyPath,
) *std.Build.Step.Run {
    const abi = switch (target.result.cpu.arch) {
        .aarch64 => "arm64-v8a",
        .x86_64 => "x86_64",
        else => @panic("Android test runner supports ARM64 and x64 targets only"),
    };
    const run_tests = b.addSystemCommand(&.{
        "bash",
        android_runner.getPath(b),
        "180",
        abi,
        installPath(b, native_install_dir, "lib/libmaplibre-native-c.so").getPath(b),
        "--",
    });
    for (tests) |test_executable| {
        run_tests.addArtifactArg(test_executable);
    }
    return run_tests;
}

pub fn build(b: *std.Build) void {
    const native_install_dir = maybeNativeInstallDirPath(b);
    const target_libc = takeTargetLibCPath(b);
    const target = if (native_install_dir) |install_dir|
        nativeTarget(b, install_dir)
    else
        b.standardTargetOptions(.{});
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

    const required_native_install_dir = native_install_dir orelse return;
    const dependency_include_dirs = dependencyIncludeDirs(b);
    const dependency_library_dirs = dependencyLibraryDirs(b);
    const system_root = maybeSystemRootPath(b);

    const backend = renderBackend(b, required_native_install_dir);
    const options = BuildOptions{
        .target = target,
        .optimize = testOptimize(target, optimize),
        .native_install_dir = required_native_install_dir,
        .include_dirs = installedIncludeDirs(b, required_native_install_dir, dependency_include_dirs),
        .dependency_library_dirs = dependency_library_dirs,
        .render_backend = backend,
        .system_root = system_root,
        .target_libc = target_libc,
    };

    const maplibre_native_ffi = addMaplibreNativeModule(b, options);

    const test_sources = [_]std.Build.LazyPath{
        b.path("bindings/zig/src/status.zig"),
        b.path("bindings/zig/src/runtime.zig"),
        b.path("bindings/zig/src/logging.zig"),
        b.path("bindings/zig/src/map.zig"),
    };

    const test_step = b.step("test", "Run Zig binding tests");

    const binding_tests = addBindingTests(b, options, maplibre_native_ffi);
    var test_compiles: [test_sources.len + 1]*std.Build.Step.Compile = undefined;
    test_compiles[0] = binding_tests;
    b.default_step.dependOn(&binding_tests.step);

    for (test_sources, 1..) |source, index| {
        const tests = addTestCompile(b, options, source);
        test_compiles[index] = tests;
        b.default_step.dependOn(&tests.step);
    }

    if (options.target.result.abi == .android) {
        const run_tests = addAndroidTestRunStep(
            b,
            &test_compiles,
            options.target,
            options.native_install_dir,
            b.path("scripts/run-android-emulator-test.sh"),
        );
        test_step.dependOn(&run_tests.step);
    } else {
        for (test_compiles) |tests| {
            const run_tests = addTestRunStep(b, tests, options.target, b.path("scripts/run-ios-simulator-test.sh"));
            test_step.dependOn(&run_tests.step);
        }
    }
}
