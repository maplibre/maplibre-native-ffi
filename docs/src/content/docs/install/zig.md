---
title: Zig
description: Add the Zig package to a project.
---

## Install

Add the package from a release. It needs Zig 0.17.

```bash frame="terminal"
zig fetch --save https://github.com/maplibre/maplibre-native-ffi/releases/download/v{{version}}/maplibre-native-ffi-zig.tar.gz
```

```zig title="build.zig"
const maplibre = b.dependency("maplibre_native_ffi", .{
    .target = target,
    .optimize = optimize,
});
exe.root_module.addImport("maplibre_native_ffi", maplibre.module("maplibre_native_ffi"));
```

When it builds, the package downloads the C library for your target.

Your executable loads the library at run time, from its own directory on
Windows, and from `../lib` elsewhere. Install the library from the package's
`prefix` beside your executable:

```zig title="build.zig"
const library = switch (target.result.os.tag) {
    .windows => "bin/maplibre-native-c.dll",
    .macos, .maccatalyst => "lib/libmaplibre-native-c.dylib",
    else => "lib/libmaplibre-native-c.so",
};
const prefix = maplibre.namedLazyPath("prefix");
b.getInstallStep().dependOn(&b.addInstallFile(prefix.path(b, library), library).step);
```

## Choose a backend

The package uses your platform's
[default backend](/maplibre-native-ffi/install/#choose-a-backend). To choose
another, pass `.backend` to the dependency: `.opengl`, `.metal`, or `.vulkan`.

```zig title="build.zig"
const maplibre = b.dependency("maplibre_native_ffi", .{
    .target = target,
    .optimize = optimize,
    .backend = .opengl,
});
```

## Windows

Zig targets MinGW on Windows by default. To use MSVC instead, build with
`-Dtarget=x86_64-windows-msvc`.

## Mac Catalyst

Mac Catalyst builds target `aarch64-maccatalyst`.

## Android

Zig has no libc for Android, so an Android build uses the NDK's. Pass the
dependency `.@"system-root"` as the NDK's `sysroot` directory, and
`.@"target-libc"` as a libc file in the format that `zig libc` prints. The file
names `usr/include` and its target subdirectory as the include directories, and
the target's `usr/lib/<target>/<API level>` as both `crt_dir` and `cc_dir`. Call
`setLibCFile` with the same file on each artifact that imports the module. Ship
`libmaplibre-native-c.so` in the app's `lib/<abi>` directory.

Initialize the library once per process, before you create a runtime, with the
calling thread's JNI environment and an Android `Context`:

```zig title="src/main.zig"
try maplibre.initAndroid(jni_env, context, null);
```

## Snapshots

The
[snapshot release](https://github.com/maplibre/maplibre-native-ffi/releases/tag/unstable-native-snapshot)
publishes the same package, built from `main` daily. Run the same command again
to move to a newer snapshot:

```bash frame="terminal"
zig fetch --save https://github.com/maplibre/maplibre-native-ffi/releases/download/unstable-native-snapshot/maplibre-native-ffi-zig.tar.gz
```

## Use your own build

To link a C library that you built, pass its install prefix to the dependency as
`local-build`. It must be built for the same target. A path from the command
line must be absolute. A Git dependency on the package always needs one.

```zig title="build.zig"
const maplibre = b.dependency("maplibre_native_ffi", .{
    .target = target,
    .optimize = optimize,
    .@"local-build" = b.option(std.Build.LazyPath, "local-build", "MapLibre Native C library build"),
});
```
