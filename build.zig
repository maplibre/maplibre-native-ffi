const builtin = @import("builtin");
const std = @import("std");

const NativeTarget = struct {
    name: []const u8,
    platform: Platform,
    architecture: Architecture,
    backend: Backend,

    const Platform = enum { linux, macos, ios, ios_simulator, android, ohos, windows };
    const Architecture = enum { x64, arm64 };
    const Backend = enum { egl, metal, vulkan, wgl };

    fn parse(targets: []const NativeTarget, name: []const u8) ?NativeTarget {
        for (targets) |target| {
            if (std.mem.eql(u8, name, target.name)) return target;
        }
        return null;
    }

    fn default(targets: []const NativeTarget) NativeTarget {
        return switch (builtin.os.tag) {
            .linux => if (builtin.cpu.arch == .aarch64)
                parse(targets, "linux-arm64-egl").?
            else
                parse(targets, "linux-x64-egl").?,
            .macos => parse(targets, "macos-arm64-metal").?,
            .windows => if (builtin.cpu.arch == .aarch64)
                parse(targets, "windows-arm64-wgl").?
            else
                parse(targets, "windows-x64-wgl").?,
            else => @panic("pass -Dnative-target=<name> on this host"),
        };
    }

    fn zigQuery(target: NativeTarget) ?[]const u8 {
        return switch (target.platform) {
            .linux => if (target.architecture == .arm64) "aarch64-linux-gnu" else "x86_64-linux-gnu",
            .windows => if (target.architecture == .arm64) "aarch64-windows-msvc" else "x86_64-windows-msvc",
            else => null,
        };
    }
};

fn nativeTargets(b: *std.Build) []const NativeTarget {
    const path = b.pathFromRoot("native-targets.json");
    const contents = std.Io.Dir.cwd().readFileAlloc(b.graph.io, path, b.allocator, .limited(64 * 1024)) catch
        std.debug.panic("failed to read {s}", .{path});
    return std.json.parseFromSliceLeaky([]const NativeTarget, b.allocator, contents, .{}) catch |err|
        std.debug.panic("invalid native-targets.json: {s}", .{@errorName(err)});
}

fn env(b: *std.Build, name: []const u8) []const u8 {
    return b.graph.environ_map.get(name) orelse
        std.debug.panic("native target requires the {s} environment variable", .{name});
}

fn define(run: *std.Build.Step.Run, b: *std.Build, name: []const u8, value: []const u8) void {
    run.addArg(b.fmt("-D{s}={s}", .{ name, value }));
}

fn addPlatformOptions(
    b: *std.Build,
    configure: *std.Build.Step.Run,
    target: NativeTarget,
    build_dir: []const u8,
) void {
    define(configure, b, "MLN_FFI_TARGET_ARCHITECTURE", @tagName(target.architecture));

    switch (target.backend) {
        .metal => define(configure, b, "MLN_FFI_RENDER_BACKEND", "metal"),
        .vulkan => define(configure, b, "MLN_FFI_RENDER_BACKEND", "vulkan"),
        .egl => {
            define(configure, b, "MLN_FFI_RENDER_BACKEND", "opengl");
            define(configure, b, "MLN_FFI_OPENGL_CONTEXT_PROVIDER", "egl");
        },
        .wgl => {
            define(configure, b, "MLN_FFI_RENDER_BACKEND", "opengl");
            define(configure, b, "MLN_FFI_OPENGL_CONTEXT_PROVIDER", "wgl");
        },
    }

    switch (target.platform) {
        .linux => {},
        .macos, .ios, .ios_simulator => {
            define(configure, b, "CMAKE_TOOLCHAIN_FILE", b.pathFromRoot("cmake/toolchains/apple.cmake"));
            define(configure, b, "CMAKE_OSX_ARCHITECTURES", "arm64");
            define(configure, b, "CMAKE_OSX_DEPLOYMENT_TARGET", "14.3");
            define(configure, b, "CMAKE_OSX_SYSROOT", switch (target.platform) {
                .macos => "macosx",
                .ios => "iphoneos",
                .ios_simulator => "iphonesimulator",
                else => unreachable,
            });
            if (target.platform != .macos) define(configure, b, "CMAKE_SYSTEM_NAME", "iOS");
        },
        .android => {
            const toolchain = b.pathJoin(&.{
                env(b, "ANDROID_HOME"),
                "ndk",
                env(b, "MLN_FFI_ANDROID_NDK_VERSION"),
                "build",
                "cmake",
                "android.toolchain.cmake",
            });
            define(configure, b, "CMAKE_TOOLCHAIN_FILE", toolchain);
            define(configure, b, "ANDROID_PLATFORM", "android-24");
            define(configure, b, "ANDROID_STL", "c++_shared");
            define(configure, b, "ANDROID_ABI", if (target.architecture == .arm64) "arm64-v8a" else "x86_64");
            define(configure, b, "MLN_FFI_CARGO_TARGET_DIR", b.pathJoin(&.{ build_dir, "cargo" }));
        },
        .ohos => {
            define(
                configure,
                b,
                "CMAKE_TOOLCHAIN_FILE",
                b.pathJoin(&.{ env(b, "OHOS_SDK_NATIVE"), "build", "cmake", "ohos.toolchain.cmake" }),
            );
            define(configure, b, "OHOS_ARCH", "arm64-v8a");
            define(configure, b, "OHOS_STL", "c++_shared");
            define(configure, b, "MLN_FFI_ENABLE_CLANG_TIDY", "OFF");
        },
        .windows => {
            const clang_cl = b.pathJoin(&.{ env(b, "ProgramFiles"), "LLVM", "bin", "clang-cl.exe" });
            define(configure, b, "CMAKE_C_COMPILER", clang_cl);
            define(configure, b, "CMAKE_CXX_COMPILER", clang_cl);
            define(configure, b, "CMAKE_MSVC_RUNTIME_LIBRARY", "MultiThreaded");
            define(configure, b, "CMAKE_TRY_COMPILE_TARGET_TYPE", "STATIC_LIBRARY");
            define(configure, b, "HAVE_FSEEKO", "OFF");
            const triple = if (target.architecture == .arm64) "aarch64-pc-windows-msvc" else "x86_64-pc-windows-msvc";
            define(configure, b, "CMAKE_C_COMPILER_TARGET", triple);
            define(configure, b, "CMAKE_CXX_COMPILER_TARGET", triple);
        },
    }
}

fn addZigDependencies(
    b: *std.Build,
    configure: *std.Build.Step.Run,
    target: NativeTarget,
    optimize: std.builtin.OptimizeMode,
) void {
    const query_text = target.zigQuery() orelse return;
    const query = std.Target.Query.parse(.{ .arch_os_abi = query_text }) catch unreachable;
    const resolved = b.resolveTargetQuery(query);

    // This is the allyourcodebase/zlib recipe with its Unix-only define made
    // target-aware. The upstream wrapper currently defines Z_HAVE_UNISTD_H on
    // Windows too, so keeping this small recipe here makes MSVC builds work.
    const zlib_source = b.dependency("zlib_source", .{});
    const zlib = b.addLibrary(.{
        .name = "z",
        .linkage = .static,
        .root_module = b.createModule(.{
            .target = resolved,
            .optimize = optimize,
            .link_libc = true,
            .pic = true,
        }),
    });
    const zlib_flags: []const []const u8 = if (target.platform == .windows)
        &.{ "-DHAVE_SYS_TYPES_H", "-DHAVE_STDINT_H", "-DHAVE_STDDEF_H" }
    else
        &.{ "-DHAVE_SYS_TYPES_H", "-DHAVE_STDINT_H", "-DHAVE_STDDEF_H", "-DZ_HAVE_UNISTD_H" };
    zlib.root_module.addCSourceFiles(.{
        .root = zlib_source.path(""),
        .files = &.{
            "adler32.c",  "crc32.c",  "deflate.c", "infback.c",  "inffast.c", "inflate.c",
            "inftrees.c", "trees.c",  "zutil.c",   "compress.c", "uncompr.c", "gzclose.c",
            "gzlib.c",    "gzread.c", "gzwrite.c",
        },
        .flags = zlib_flags,
    });
    zlib.installHeadersDirectory(zlib_source.path(""), "", .{
        .include_extensions = &.{ "zconf.h", "zlib.h" },
    });
    configure.addPrefixedFileArg("-DMLN_FFI_ZLIB_LIBRARY=", zlib.getEmittedBin());
    configure.addPrefixedFileArg("-DMLN_FFI_ZLIB_INCLUDE_DIR=", zlib.getEmittedIncludeTree());
    configure.addPrefixedFileArg("-DMLN_FFI_ZLIB_LICENSE=", zlib_source.path("LICENSE"));

    const libuv_dep = b.dependency("libuv", .{
        .target = resolved,
        .optimize = optimize,
    });
    const libuv = libuv_dep.artifact("uv");
    configure.addPrefixedFileArg("-DMLN_FFI_LIBUV_LIBRARY=", libuv.getEmittedBin());
    configure.addPrefixedFileArg("-DMLN_FFI_LIBUV_INCLUDE_DIR=", libuv.getEmittedIncludeTree());
    configure.addPrefixedFileArg("-DMLN_FFI_LIBUV_LICENSE=", libuv_dep.path("LICENSE-LIBUV"));
}

pub fn build(b: *std.Build) void {
    const targets = nativeTargets(b);
    const target_name = b.option([]const u8, "native-target", "Native artifact target") orelse NativeTarget.default(targets).name;
    const target = NativeTarget.parse(targets, target_name) orelse
        std.debug.panic("unsupported native target: {s}", .{target_name});
    const optimize = b.option(std.builtin.OptimizeMode, "dependency-optimize", "Optimization mode for Zig-built dependencies") orelse .ReleaseFast;
    const jobs = b.option(u16, "native-jobs", "Maximum concurrent native compiler jobs") orelse 4;

    const build_dir = b.pathFromRoot(b.pathJoin(&.{ "build", target.name }));
    const install_dir = b.pathJoin(&.{ build_dir, "install" });

    const configure = b.addSystemCommand(&.{
        "cmake",
        "-S",
        b.pathFromRoot("."),
        "-B",
        build_dir,
        "-G",
        "Ninja",
    });
    configure.setName(b.fmt("configure {s}", .{target.name}));
    configure.stdio = .inherit;
    define(configure, b, "CMAKE_BUILD_TYPE", "Release");
    define(configure, b, "CMAKE_INSTALL_PREFIX", install_dir);
    define(configure, b, "CMAKE_INSTALL_LIBDIR", "lib");
    define(configure, b, "MLN_FFI_ARTIFACT_NAME", target.name);
    define(configure, b, "MLN_FFI_BUILD_JOBS", b.fmt("{d}", .{jobs}));
    if (target.platform == .windows and target.architecture == .arm64) {
        define(configure, b, "BUILD_TESTING", "OFF");
    }
    addPlatformOptions(b, configure, target, build_dir);
    if (target.platform == .linux or target.platform == .windows) {
        addZigDependencies(b, configure, target, optimize);
    }

    const configure_step = b.step("configure", "Configure the selected native target");
    configure_step.dependOn(&configure.step);

    const install = b.addSystemCommand(&.{ "cmake", "--build", build_dir, "--target", "install", "--parallel" });
    install.addArg(b.fmt("{d}", .{jobs}));
    install.setName(b.fmt("build {s}", .{target.name}));
    install.stdio = .inherit;
    install.step.dependOn(&configure.step);
    b.getInstallStep().dependOn(&install.step);

    const test_command = b.addSystemCommand(&.{
        "ctest",
        "--test-dir",
        build_dir,
        "--output-on-failure",
        "--timeout",
        "300",
    });
    test_command.setName(b.fmt("test {s}", .{target.name}));
    test_command.stdio = .inherit;
    if (target.platform == .linux and target.backend == .egl) {
        test_command.setEnvironmentVariable("EGL_PLATFORM", "surfaceless");
        test_command.setEnvironmentVariable("LIBGL_ALWAYS_SOFTWARE", "true");
    }
    test_command.step.dependOn(&install.step);
    const test_step = b.step("test", "Build and run the C API tests");
    test_step.dependOn(&test_command.step);

    const package = b.addSystemCommand(&.{
        "cpack",
        "--config",
        b.pathJoin(&.{ build_dir, "CPackConfig.cmake" }),
        "-G",
        "TGZ",
    });
    package.setName(b.fmt("package {s}", .{target.name}));
    package.stdio = .inherit;
    package.step.dependOn(&install.step);
    const package_step = b.step("package", "Build and archive the selected native target");
    package_step.dependOn(&package.step);

    const clean = b.addSystemCommand(&.{ "cmake", "-E", "remove_directory", build_dir });
    clean.setName(b.fmt("clean {s}", .{target.name}));
    clean.stdio = .inherit;
    const clean_step = b.step("clean", "Remove the selected native build tree");
    clean_step.dependOn(&clean.step);
}
