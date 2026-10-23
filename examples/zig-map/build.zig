const std = @import("std");
const maplibre_build = @import("maplibre_native_ffi");
const zigglgen = @import("zigglgen");

const BuildOptions = struct {
    native: maplibre_build.Native,
    optimize: std.builtin.OptimizeMode,
    include_dirs: []const std.Build.LazyPath,
    dependency_include_dirs: []const std.Build.LazyPath,
    dependency_library_dirs: []const std.Build.LazyPath,
    system_root: ?std.Build.LazyPath,
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
    const target = options.native.target;
    const sdl = b.dependency("sdl", .{
        .target = target,
        .optimize = options.optimize,
    });
    const library = sdl.artifact("SDL3");
    if (target.result.os.tag == .macos) {
        if (options.system_root) |system_root| {
            library.root_module.addSystemIncludePath(system_root.path(b, b.pathJoin(&.{ "usr", "include" })));
            library.root_module.addLibraryPath(system_root.path(b, b.pathJoin(&.{ "usr", "lib" })));
            library.root_module.addSystemFrameworkPath(system_root.path(b, b.pathJoin(&.{ "System", "Library", "Frameworks" })));
        }
    }
    return library;
}

fn sdlTranslateCMacros(target: std.Build.ResolvedTarget) []const maplibre_build.CMacro {
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

fn maplibreNativeModule(b: *std.Build, options: BuildOptions) *std.Build.Module {
    return b.dependency("maplibre_native_ffi", .{
        .target = options.native.target,
        .optimize = options.optimize,
        .@"local-build" = options.native.prefix,
        .backend = options.native.backend,
        .@"dependency-include-dir" = options.dependency_include_dirs,
        .@"system-root" = options.system_root,
    }).module("maplibre_native_ffi");
}

fn cBindingsHeader(b: *std.Build, backend: maplibre_build.RenderBackend) std.Build.LazyPath {
    return switch (backend) {
        .metal => b.path("c_metal.h"),
        .opengl => b.path("c_opengl.h"),
        .vulkan => b.path("c_vulkan.h"),
    };
}

fn addDependencyLibraryPaths(module: *std.Build.Module, dependency_library_dirs: []const std.Build.LazyPath) void {
    for (dependency_library_dirs) |dependency_library_dir| {
        module.addLibraryPath(dependency_library_dir);
        module.addRPath(dependency_library_dir);
    }
}

/// Finds the Vulkan loader on macOS and Windows, which have no system copy.
fn addNativeVulkanSdkPath(b: *std.Build, module: *std.Build.Module, options: BuildOptions) void {
    const target = options.native.target;
    if (options.dependency_library_dirs.len != 0) return;
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

/// Links the render backend's graphics libraries, which the example calls
/// itself.
fn linkRenderBackend(b: *std.Build, module: *std.Build.Module, options: BuildOptions) void {
    const target = options.native.target;
    maplibre_build.addPlatformSystemPaths(b, module, target, options.system_root);
    switch (options.native.backend) {
        .metal => {
            module.linkFramework("Metal", .{});
            module.linkFramework("QuartzCore", .{});
        },
        .opengl => switch (target.result.os.tag) {
            .linux, .macos => {
                addDependencyLibraryPaths(module, options.dependency_library_dirs);
                module.linkSystemLibrary("EGL", .{});
                module.linkSystemLibrary("GLESv2", .{});
            },
            .windows => module.linkSystemLibrary("opengl32", .{}),
            else => unreachable,
        },
        .vulkan => {
            addNativeVulkanSdkPath(b, module, options);
            addDependencyLibraryPaths(module, options.dependency_library_dirs);
            module.linkSystemLibrary(if (target.result.os.tag == .windows) "vulkan-1" else "vulkan", .{});
        },
    }
}

fn addZigMapExample(b: *std.Build, options: BuildOptions) *std.Build.Step.Compile {
    const target = options.native.target;
    const backend = options.native.backend;
    const root_module = b.createModule(.{
        .root_source_file = b.path("main.zig"),
        .target = target,
        .optimize = options.optimize,
    });
    const example = b.addExecutable(.{
        .name = "zig-map",
        .root_module = root_module,
    });

    const sdl = sdlLibrary(b, options);
    const c_include_dirs = appendIncludeDir(b, options.include_dirs, sdl.getEmittedIncludeTree());

    maplibre_build.addRenderBackendOptions(b, root_module, backend);
    root_module.addImport("c", maplibre_build.translateCModule(b, .{
        .root_source_file = cBindingsHeader(b, backend),
        .target = target,
        .optimize = options.optimize,
        .include_dirs = c_include_dirs,
        .c_macros = sdlTranslateCMacros(target),
        .system_root = options.system_root,
    }));
    root_module.addImport("maplibre_native_ffi", maplibreNativeModule(b, options));
    if (backend == .opengl) {
        const gl_bindings = zigglgen.generateBindingsModule(b, if (target.result.os.tag == .linux or target.result.os.tag == .macos)
            .{ .api = .gles, .version = .@"3.0" }
        else
            .{ .api = .gl, .version = .@"3.0" });
        root_module.addImport("gl", gl_bindings);
    }
    root_module.linkLibrary(sdl);
    linkRenderBackend(b, root_module, options);

    if (backend == .metal) {
        const zig_objc = b.dependency("zig_objc", .{
            .target = target,
            .optimize = options.optimize,
        });
        root_module.addImport("objc", zig_objc.module("objc"));
        root_module.linkFramework("Foundation", .{});
    }

    if (backend == .vulkan) {
        var previous: ?*std.Build.Step = null;
        for ([2][]const u8{ "fullscreen.vert", "sample.frag" }) |shader| {
            const compile_shader = b.addSystemCommand(&.{ "glslangValidator", "-V" });
            compile_shader.addFileArg(b.path(b.pathJoin(&.{ "render/vulkan/shaders", shader })));
            compile_shader.addArgs(&.{ "-o", b.pathJoin(&.{ "render/vulkan/shaders", b.fmt("{s}.spv", .{shader}) }) });
            if (previous) |step| compile_shader.step.dependOn(step);
            previous = &compile_shader.step;
        }
        example.step.dependOn(previous.?);
    }

    b.installArtifact(example);
    return example;
}

pub fn build(b: *std.Build) void {
    const native = maplibre_build.localNative(b);
    const dependency_include_dirs = b.option([]const std.Build.LazyPath, "dependency-include-dir", "Additional local dependency include directory") orelse &.{};
    const options = BuildOptions{
        .native = native,
        .optimize = b.standardOptimizeOption(.{}),
        .include_dirs = appendIncludeDir(b, dependency_include_dirs, native.prefix.path(b, "include")),
        .dependency_include_dirs = dependency_include_dirs,
        .dependency_library_dirs = b.option([]const std.Build.LazyPath, "dependency-library-dir", "Additional local dependency library directory") orelse &.{},
        .system_root = b.option(std.Build.LazyPath, "system-root", "Target platform SDK or sysroot"),
    };

    const run_step = b.step("run", "Run Zig map example");
    const zig_map = addZigMapExample(b, options);
    const run_zig_map = b.addRunArtifact(zig_map);
    run_zig_map.addPassthruArgs();
    run_step.dependOn(&run_zig_map.step);
}
