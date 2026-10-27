// This build script serves both the repository and the package that each
// release publishes. The package holds this file and bindings/zig/src at the
// same paths, with a build.zig.zon that declares each native archive of the
// release as a lazy dependency. A checkout declares none, so it builds against
// -Dlocal-build.

const std = @import("std");
const Translator = @import("translate_c").Translator;

pub const RenderBackend = enum {
    metal,
    opengl,
    vulkan,
};

/// The fields of a native artifact descriptor that select an install, which
/// a release pin's artifact index repeats for each archive.
const ArtifactDescriptor = struct {
    artifact: []const u8,
    renderBackend: []const u8,
    defaultBackend: []const u8,
    zigTarget: []const u8,
};

const descriptor_sub_path = "share/maplibre-native-c/artifact.json";
const pin_sub_path = "bindings/zig/native_release.json";

/// A native install prefix and the target and render backend it serves.
pub const Native = struct {
    prefix: std.Build.LazyPath,
    target: std.Build.ResolvedTarget,
    backend: RenderBackend,
};

const BuildOptions = struct {
    native: Native,
    optimize: std.builtin.OptimizeMode,
    include_dirs: []const std.Build.LazyPath,
    system_root: ?std.Build.LazyPath,
    libc_file: ?std.Build.LazyPath,
};

fn parseRenderBackend(value: []const u8) RenderBackend {
    return std.meta.stringToEnum(RenderBackend, value) orelse
        std.debug.panic("unsupported render backend: {s}", .{value});
}

/// The filesystem path of a path option, which configuration reads before any
/// step runs.
fn configurePath(b: *std.Build, path: std.Build.LazyPath) []const u8 {
    return switch (path) {
        .cwd_relative => |p| p,
        .src_path => |p| p.owner.root.joinString(b.allocator, p.sub_path) catch @panic("OOM"),
        .dependency => |p| p.dependency.builder.root.joinString(b.allocator, p.sub_path) catch @panic("OOM"),
        else => std.debug.panic("expected a path to an existing directory, got {f}", .{path}),
    };
}

/// An install prefix and its artifact descriptor.
const Install = struct {
    prefix: []const u8,
    descriptor: ArtifactDescriptor,
};

/// Reads the install prefix that -Dlocal-build names, and rebuilds when its
/// descriptor changes.
fn localInstall(b: *std.Build, prefix: []const u8) Install {
    // A relative path would reach the run-time search path as written.
    if (!std.fs.path.isAbsolute(prefix)) std.debug.panic("-Dlocal-build must be an absolute path; found {s}", .{prefix});
    const path = b.pathJoin(&.{ prefix, descriptor_sub_path });
    const contents = std.Io.Dir.cwd().readFileAlloc(b.graph.io, path, b.allocator, .limited(4096)) catch |err| switch (err) {
        error.FileNotFound, error.NotDir => std.debug.panic("-Dlocal-build names {s}, which is not an install prefix because it lacks {s}", .{ prefix, descriptor_sub_path }),
        else => std.debug.panic("failed to read {s}: {t}", .{ path, err }),
    };
    const parsed = std.json.parseFromSlice(ArtifactDescriptor, b.allocator, contents, .{ .ignore_unknown_fields = true }) catch
        std.debug.panic("invalid native artifact descriptor: {s}", .{path});
    b.dependOnFileContents(.{ .cwd_relative = path });
    return .{ .prefix = prefix, .descriptor = parsed.value };
}

/// The target that an artifact's Zig target names. An Android API level
/// suffix does not count.
fn artifactQuery(descriptor: ArtifactDescriptor) std.Target.Query {
    const query = std.Target.Query.parse(.{ .arch_os_abi = descriptor.zigTarget }) catch
        std.debug.panic("invalid Zig target in native artifact descriptor: {s}", .{descriptor.zigTarget});
    if (query.cpu_arch == null or query.os_tag == null) {
        std.debug.panic("invalid Zig target in native artifact descriptor: {s}", .{descriptor.zigTarget});
    }
    return query;
}

fn servesTarget(descriptor: ArtifactDescriptor, target: std.Target) bool {
    const query = artifactQuery(descriptor);
    const abi = query.abi orelse std.Target.Abi.default(query.cpu_arch.?, query.os_tag.?);
    return query.cpu_arch.? == target.cpu.arch and query.os_tag.? == target.os.tag and abi == target.abi;
}

/// Whether an artifact has the selected backend, or with none selected, its
/// platform's default backend.
fn servesBackend(descriptor: ArtifactDescriptor, backend: ?RenderBackend) bool {
    const wanted = if (backend) |selected| @tagName(selected) else descriptor.defaultBackend;
    return std.mem.eql(u8, descriptor.renderBackend, wanted);
}

fn requestName(b: *std.Build, target: std.Target, backend: ?RenderBackend) []const u8 {
    return b.fmt("{t}-{t}-{t} with {s}", .{
        target.cpu.arch,
        target.os.tag,
        target.abi,
        if (backend) |selected| b.fmt("the {t} backend", .{selected}) else "its default backend",
    });
}

/// The target option, which defaults to the target of an install prefix's
/// artifact when it differs from the host.
fn targetOption(b: *std.Build, descriptor: ?ArtifactDescriptor) std.Build.ResolvedTarget {
    const artifact = descriptor orelse return b.standardTargetOptions(.{});
    const artifact_query = artifactQuery(artifact);
    const artifact_target = b.resolveTargetQuery(artifact_query).result;
    const host = b.graph.host.result;
    const native_host = artifact_target.cpu.arch == host.cpu.arch and
        artifact_target.os.tag == host.os.tag and
        artifact_target.abi == host.abi;
    return b.standardTargetOptions(.{ .default_target = if (native_host) .{} else artifact_query });
}

/// The release archive for the request, which the published package declares
/// as a lazy dependency named by the archive's artifact. Zig fetches it when a
/// build first asks for it.
fn publishedNative(b: *std.Build, target: std.Target, backend: ?RenderBackend) struct { std.Build.LazyPath, ArtifactDescriptor } {
    const pin_path = b.path(pin_sub_path);
    const pin_file = configurePath(b, pin_path);
    const contents = std.Io.Dir.cwd().readFileAlloc(b.graph.io, pin_file, b.allocator, .unlimited) catch |err| switch (err) {
        error.FileNotFound => @panic("maplibre_native_ffi has no native release pin. Only the package from a release downloads the native library; otherwise pass -Dlocal-build."),
        else => std.debug.panic("failed to read {s}: {t}", .{ pin_file, err }),
    };
    b.dependOnFileContents(pin_path);
    const pin = std.json.parseFromSlice(struct {
        tag: []const u8,
        artifacts: []const ArtifactDescriptor,
    }, b.allocator, contents, .{ .ignore_unknown_fields = true }) catch
        std.debug.panic("invalid native release pin: {s}", .{pin_file});
    var chosen: ?ArtifactDescriptor = null;
    for (pin.value.artifacts) |entry| {
        if (!servesTarget(entry, target) or !servesBackend(entry, backend)) continue;
        if (chosen != null) std.debug.panic("the native release pin lists several archives for {s}", .{requestName(b, target, backend)});
        chosen = entry;
    }
    const entry = chosen orelse std.debug.panic("release {s} has no native archive for {s}; pass -Dlocal-build to use your own build", .{
        pin.value.tag,
        requestName(b, target, backend),
    });
    return .{ b.dependency(entry.artifact, .{}).path(""), entry };
}

/// Declares the -Dtarget, -Dbackend, and -Dlocal-build options and resolves
/// the native library they select: from the local build if one is named, and
/// otherwise from the release that the package was published with.
fn resolveNative(b: *std.Build, local_build: ?std.Build.LazyPath) Native {
    const local = if (local_build) |path| localInstall(b, configurePath(b, path)) else null;
    const target = targetOption(b, if (local) |install| install.descriptor else null);
    switch (target.result.os.tag) {
        .ios, .tvos, .visionos => @panic("The Zig binding does not support iOS, tvOS, or visionOS targets, including simulators"),
        .emscripten => @panic("The Zig binding does not support Emscripten"),
        else => {},
    }
    const selected_backend = b.option(
        RenderBackend,
        "backend",
        "Render backend. Defaults to the target's default, which the native artifact names.",
    );

    const prefix: std.Build.LazyPath, const descriptor = if (local) |install| local: {
        // A prefix serves one target, with its own backend unless one is
        // selected.
        const descriptor = install.descriptor;
        if (!servesTarget(descriptor, target.result) or (selected_backend != null and !servesBackend(descriptor, selected_backend))) {
            std.debug.panic("-Dlocal-build names an install prefix built by {s}, which cannot serve {s}", .{
                descriptor.artifact,
                requestName(b, target.result, selected_backend),
            });
        }
        break :local .{ .{ .cwd_relative = install.prefix }, descriptor };
    } else publishedNative(b, target.result, selected_backend);
    return .{
        .prefix = prefix,
        .target = target,
        .backend = parseRenderBackend(descriptor.renderBackend),
    };
}

/// Resolves the native library from a required -Dlocal-build, for a build in
/// this repository that depends on this package by path.
pub fn localNative(b: *std.Build) Native {
    const local_build = localBuildOption(b) orelse
        @panic("missing required -Dlocal-build=<install prefix>");
    return resolveNative(b, local_build);
}

fn localBuildOption(b: *std.Build) ?std.Build.LazyPath {
    return b.option(
        std.Build.LazyPath,
        "local-build",
        "Absolute path to a native install prefix",
    );
}

pub const CMacro = struct {
    name: []const u8,
    value: ?[]const u8 = null,
};

pub const TranslateCModuleOptions = struct {
    root_source_file: std.Build.LazyPath,
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
    include_dirs: []const std.Build.LazyPath,
    c_macros: []const CMacro = &.{},
    system_root: ?std.Build.LazyPath = null,
    libc_file: ?std.Build.LazyPath = null,
};

pub fn translateCModule(b: *std.Build, options: TranslateCModuleOptions) *std.Build.Module {
    const translate_c_dep = b.dependency("translate_c", .{});
    const translator: Translator = .init(translate_c_dep, .{
        .c_source_file = options.root_source_file,
        .target = options.target,
        .optimize = options.optimize,
        .libc_file = options.libc_file,
    });
    for (options.include_dirs) |include_dir| {
        translator.addIncludePath(include_dir);
    }
    addPlatformSystemHeaderPaths(b, &translator, options.target, options.system_root);
    if (options.target.result.abi.isAndroid()) {
        // Zig's C translator needs the API level from the resolved Android target.
        translator.defineCMacro("__ANDROID_MIN_SDK_VERSION__", b.fmt("{d}", .{options.target.result.os.version_range.linux.android}));
    }
    for (options.c_macros) |c_macro| {
        translator.defineCMacro(c_macro.name, c_macro.value);
    }
    return translator.mod;
}

fn maplibreNativeCHeader(b: *std.Build) std.Build.LazyPath {
    const header = b.addWriteFiles();
    // Layer plugin registration stays outside the umbrella header; see
    // include/maplibre_native_c/plugin.h.
    return header.add("maplibre_native_c_import.h",
        \\#include <maplibre_native_c.h>
        \\#include <maplibre_native_c/plugin.h>
        \\
    );
}

fn addPlatformSystemHeaderPaths(b: *std.Build, destination: anytype, target: std.Build.ResolvedTarget, system_root: ?std.Build.LazyPath) void {
    if (!target.result.os.tag.isDarwin() and target.result.os.tag != .linux) return;
    const root = system_root orelse return;

    if (target.result.os.tag.isDarwin()) {
        destination.addSystemFrameworkPath(root.path(b, b.pathJoin(&.{ "System", "Library", "Frameworks" })));
    }
    destination.addSystemIncludePath(root.path(b, b.pathJoin(&.{ "usr", "include" })));
}

pub fn addPlatformSystemPaths(b: *std.Build, module: *std.Build.Module, target: std.Build.ResolvedTarget, system_root: ?std.Build.LazyPath) void {
    addPlatformSystemHeaderPaths(b, module, target, system_root);
    if (!target.result.os.tag.isDarwin() and target.result.os.tag != .linux) return;
    // Android's libraries live under a target- and API-specific directory that
    // the target libc installation names.
    if (target.result.abi == .android) return;
    const root = system_root orelse return;

    module.addLibraryPath(root.path(b, b.pathJoin(&.{ "usr", "lib" })));
    if (target.result.os.tag == .linux) {
        module.addLibraryPath(root.path(b, b.pathJoin(&.{ "usr", "lib64" })));
        module.addLibraryPath(root.path(b, "lib"));
        module.addLibraryPath(root.path(b, "lib64"));
    }
}

pub fn addRenderBackendOptions(b: *std.Build, module: *std.Build.Module, backend: RenderBackend) void {
    const build_options = b.addOptions();
    build_options.addOption(bool, "supports_metal", backend == .metal);
    build_options.addOption(bool, "supports_opengl", backend == .opengl);
    build_options.addOption(bool, "supports_vulkan", backend == .vulkan);
    module.addOptions("build_options", build_options);
}

/// Imports the C declarations as `maplibre_native_c`, without linking
/// maplibre-native-c.
fn addMaplibreNativeIncludes(
    b: *std.Build,
    module: *std.Build.Module,
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
    include_dirs: []const std.Build.LazyPath,
    system_root: ?std.Build.LazyPath,
    libc_file: ?std.Build.LazyPath,
) void {
    for (include_dirs) |include_dir| {
        module.addIncludePath(include_dir);
    }
    module.link_libc = true;
    module.addImport("maplibre_native_c", translateCModule(b, .{
        .root_source_file = maplibreNativeCHeader(b),
        .target = target,
        .optimize = optimize,
        .include_dirs = include_dirs,
        .system_root = system_root,
        .libc_file = libc_file,
    }));
}

/// Links a module to maplibre-native-c alone. The library loads its render
/// backend's graphics libraries at run time, so a module that calls them
/// itself links them.
fn linkMaplibreNativeC(b: *std.Build, module: *std.Build.Module, options: BuildOptions) void {
    const target = options.native.target;
    const prefix = options.native.prefix;
    addMaplibreNativeIncludes(b, module, target, options.optimize, options.include_dirs, options.system_root, options.libc_file);
    if (target.result.os.tag == .windows) {
        module.addObjectFile(prefix.path(b, "lib/maplibre-native-c.lib"));
    } else {
        const lib_dir = prefix.path(b, "lib");
        module.addLibraryPath(lib_dir);
        // An installed executable finds the library that its build installs
        // into lib/.
        module.addRPathSpecial(if (target.result.os.tag.isDarwin()) "@loader_path/../lib" else "$ORIGIN/../lib");
        // An executable run from the build tree finds it where the build
        // linked it. Zig gives a fetched package's path relative to the
        // working directory, which a run-time search path cannot be.
        const cwd = std.process.currentPathAlloc(b.graph.io, b.allocator) catch |err|
            std.debug.panic("failed to read the working directory: {t}", .{err});
        module.addRPath(.{ .cwd_relative = b.pathResolve(&.{ cwd, configurePath(b, lib_dir) }) });
        module.linkSystemLibrary("maplibre-native-c", .{ .use_pkg_config = .no });
    }
    addPlatformSystemPaths(b, module, target, options.system_root);
}

fn addMaplibreNativeDocs(
    b: *std.Build,
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
    include_dirs: []const std.Build.LazyPath,
    libc_file: ?std.Build.LazyPath,
) void {
    const docs_module = b.createModule(.{
        .root_source_file = b.path("bindings/zig/src/maplibre_native_ffi.zig"),
        .target = target,
        .optimize = optimize,
    });
    addMaplibreNativeIncludes(b, docs_module, target, optimize, include_dirs, null, libc_file);

    const doc_compile = b.addObject(.{
        .name = "maplibre_native_ffi_docs",
        .root_module = docs_module,
    });
    doc_compile.setLibCFile(libc_file);
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
            .target = options.native.target,
            .optimize = options.optimize,
        }),
    });
    tests.setLibCFile(options.libc_file);
    linkMaplibreNativeC(b, tests.root_module, options);
    return tests;
}

/// Gives a test module the GPU fixtures of tests/graphics as the
/// `mln_test_graphics` import. Tests load the shared library that
/// `mise run build` installs beside maplibre-native-c.
fn addTestGraphics(b: *std.Build, module: *std.Build.Module, options: BuildOptions) void {
    const include_dir = b.path("tests/graphics/include");
    module.addIncludePath(include_dir);
    module.addImport("mln_test_graphics", translateCModule(b, .{
        .root_source_file = b.path("tests/graphics/include/mln_test_graphics.h"),
        .target = options.native.target,
        .optimize = options.optimize,
        .include_dirs = &.{include_dir},
        .system_root = options.system_root,
        .libc_file = options.libc_file,
    }));
    if (options.native.target.result.os.tag == .windows) {
        module.addObjectFile(options.native.prefix.path(b, "lib/mln_test_graphics.lib"));
    } else {
        module.linkSystemLibrary("mln_test_graphics", .{ .use_pkg_config = .no });
    }
}

/// An executable that leaves a runtime, a map, and process-global callbacks
/// live when it returns, so the test step checks that the process still exits
/// cleanly.
fn addShutdownProbe(b: *std.Build, options: BuildOptions, maplibre_native_ffi: *std.Build.Module) *std.Build.Step.Compile {
    const probe = b.addExecutable(.{
        .name = "zig-shutdown-probe",
        .root_module = b.createModule(.{
            .root_source_file = b.path("bindings/zig/tests/shutdown_probe.zig"),
            .target = options.native.target,
            .optimize = options.optimize,
        }),
    });
    probe.setLibCFile(options.libc_file);
    linkMaplibreNativeC(b, probe.root_module, options);
    probe.root_module.addImport("maplibre_native_ffi", maplibre_native_ffi);
    return probe;
}

fn addBindingTests(b: *std.Build, options: BuildOptions, maplibre_native_ffi: *std.Build.Module) *std.Build.Step.Compile {
    const tests = addTestCompile(b, options, b.path("bindings/zig/tests/main.zig"));
    tests.root_module.addImport("maplibre_native_ffi", maplibre_native_ffi);
    addTestGraphics(b, tests.root_module, options);
    addRenderBackendOptions(b, tests.root_module, options.native.backend);
    return tests;
}

/// Runs each test executable through tools/devices/run, which finds the
/// device for the install and pushes the executable there. One device runs one
/// executable at a time, so each run waits for the one before it.
fn addDeviceTestRunSteps(b: *std.Build, test_step: *std.Build.Step, tests: []const *std.Build.Step.Compile, native: Native) void {
    const install = configurePath(b, native.prefix);
    var previous: ?*std.Build.Step = null;
    for (tests) |test_executable| {
        const run_tests = std.Build.Step.Run.create(b, b.fmt("run {s} on its device", .{test_executable.name}));
        run_tests.addFileArg(b.path("tools/devices/run"));
        run_tests.addArtifactArg(test_executable);
        run_tests.setEnvironmentVariable("MLN_FFI_DEVICE_INSTALL", install);
        // --test-timeout bounds only tests that run in process, so the runner
        // bounds each executable instead.
        run_tests.setEnvironmentVariable("MLN_FFI_DEVICE_TIMEOUT", "300");
        // An emulator receives the graphics fixtures beside the executable.
        run_tests.setEnvironmentVariable("MLN_FFI_DEVICE_LIBRARIES", b.pathJoin(&.{ install, "lib/libmln_test_graphics.so" }));
        if (previous) |step| run_tests.step.dependOn(step);
        previous = &run_tests.step;
        test_step.dependOn(&run_tests.step);
    }
}

fn addTests(b: *std.Build, options: BuildOptions, maplibre_native_ffi: *std.Build.Module) void {
    // The hand-written runtime modules' inline tests build and run on their
    // own, from one root that reaches each module once.
    const test_sources = [_]std.Build.LazyPath{
        b.path("bindings/zig/src/runtime_tests.zig"),
    };

    const test_step = b.step("test", "Run Zig binding tests");

    const binding_tests = addBindingTests(b, options, maplibre_native_ffi);
    const shutdown_probe = addShutdownProbe(b, options, maplibre_native_ffi);
    var test_compiles: [test_sources.len + 2]*std.Build.Step.Compile = undefined;
    test_compiles[0] = binding_tests;
    test_compiles[1] = shutdown_probe;
    b.default_step.dependOn(&binding_tests.step);
    b.default_step.dependOn(&shutdown_probe.step);

    for (test_sources, 2..) |source, index| {
        const tests = addTestCompile(b, options, source);
        test_compiles[index] = tests;
        b.default_step.dependOn(&tests.step);
    }

    const target = options.native.target.result;
    if (target.abi == .android and target.cpu.arch == .arm) {
        const unsupported = b.addFail("The Android emulator runs ARM64 and x64 guests; build this target without the test step.");
        test_step.dependOn(&unsupported.step);
    } else if (target.abi == .android or target.abi == .musl or target.os.tag == .freebsd) {
        // Android executables run on an emulator, musl executables load
        // Alpine's graphics stack through musl's dynamic loader in an Alpine
        // container, and FreeBSD executables run in a FreeBSD virtual machine.
        addDeviceTestRunSteps(b, test_step, &test_compiles, options.native);
    } else {
        for (test_compiles) |tests| {
            const run_tests = b.addRunArtifact(tests);
            test_step.dependOn(&run_tests.step);
        }
    }
}

pub fn build(b: *std.Build) void {
    const optimize = b.standardOptimizeOption(.{});
    // Target executables use this installation; host tools use the host libc.
    const libc_file = b.option(std.Build.LazyPath, "target-libc", "Target libc installation file");
    const system_root = b.option(std.Build.LazyPath, "system-root", "Target platform SDK or sysroot");
    const dependency_include_dirs = b.option([]const std.Build.LazyPath, "dependency-include-dir", "Additional local dependency include directory") orelse &.{};
    const local_build = localBuildOption(b);
    // The documentation reads the checkout's headers, so it needs no native
    // library.
    const doc_include_dirs = b.pathList(&.{
        "include",
        // The plugin header includes MapLibre Native's mln/plugin/plugin_api.h.
        "third_party/maplibre-native/include",
    });

    // A checkout without a local build has no native library to link.
    if (local_build == null and b.isRoot()) {
        addMaplibreNativeDocs(b, b.standardTargetOptions(.{}), optimize, doc_include_dirs, libc_file);
        return;
    }

    const native = resolveNative(b, local_build);
    const include_dirs = b.allocator.alloc(std.Build.LazyPath, dependency_include_dirs.len + 1) catch @panic("OOM");
    include_dirs[0] = native.prefix.path(b, "include");
    @memcpy(include_dirs[1..], dependency_include_dirs);
    const options = BuildOptions{
        .native = native,
        .optimize = optimize,
        .include_dirs = include_dirs,
        .system_root = system_root,
        .libc_file = libc_file,
    };

    const maplibre_native_ffi = b.addModule("maplibre_native_ffi", .{
        .root_source_file = b.path("bindings/zig/src/maplibre_native_ffi.zig"),
        .target = native.target,
        .optimize = optimize,
    });
    linkMaplibreNativeC(b, maplibre_native_ffi, options);
    // An installed executable finds the library beside itself on Windows and
    // in ../lib elsewhere, so its build installs the library from here.
    b.addNamedLazyPath("prefix", native.prefix);

    // The published package carries neither the tests nor the headers that
    // the documentation reads.
    if (!b.isRoot()) return;
    addMaplibreNativeDocs(b, native.target, optimize, doc_include_dirs, libc_file);
    addTests(b, options, maplibre_native_ffi);
}
