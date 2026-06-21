---
title: Native build north star
description: Target architecture for native core builds and internal binding consumption.
sidebar:
  order: 5
---

This document describes the target shape for building and consuming the native
MapLibre Native FFI core inside this repository. Public packaging is outside
this scope.

## Goals

Native builds should have one source of truth for platform, architecture, render
backend, linkage, and runtime dependency metadata. Bindings and examples should
consume that metadata instead of reconstructing native linker and loader state
in each language build system.

The native core should support two first-class internal artifact shapes:

- `static-monolithic` for native final binaries.
- `shared-private` for separate language runtimes that load native code at
  runtime.

Both artifact shapes should come from the same native implementation graph.
Differences between them should be final artifact policy, not separate build
systems.

## Artifact Model

The native build graph should produce an internal implementation aggregate that
contains the C API implementation, MapLibre Native core objects, and private
vendored native dependencies that can be bundled safely.

Final artifacts should be built from that aggregate:

```text
native implementation aggregate
        |
        +-- static-monolithic
        |
        +-- shared-private
```

`static-monolithic` is the preferred artifact for Zig, Rust native binaries,
Swift native binaries, Kotlin/Native binaries, and other consumers that produce
one final native executable or app binary.

`shared-private` is the preferred artifact for JVM, .NET, Kotlin/JVM, and other
runtime boundaries that need `dlopen`, `System.loadLibrary`, `NativeLibrary`, or
equivalent dynamic loading.

The shared artifact should not expose MapLibre Native, ICU, HarfBuzz, Freetype,
SQLite, CURL, or other private dependency symbols as process-visible API. The
only intended exported ABI surface is the `mln_*` C API.

## Static Monolithic

Static monolithic artifacts should bundle the native implementation and private
static dependencies into one archive when the platform toolchain supports that
cleanly.

Static consumers should still receive machine-readable link metadata for
remaining platform dependencies. Examples include Apple frameworks, Android and
OHOS system libraries, Vulkan loader libraries, EGL/GLES/OpenGL libraries,
`libc++`, `z`, and `sqlite3`.

Bindings should not hardcode MapLibre Native vendor archive names. If a final
static link requires `mbgl-core`, `mbgl-vendor-*`, or other implementation
archives, the native build should report those requirements through the shared
metadata contract.

## Shared Private

Shared private artifacts should be built with strict symbol export control.

The build should use platform-native export controls:

- macOS and iOS: exported symbols lists.
- Linux, Android, and OHOS: linker version scripts.
- Windows: explicit exports through declarations or module definition files.

Shared private artifacts should load with their private runtime dependencies
resolved predictably from the build output or platform runtime environment.
Bindings may provide an exact library path override, but they should share the
same override name and lookup semantics where their runtimes allow it.

## Metadata Contract

Each configured native variant should produce a generated metadata file in the
variant build directory. That file should describe the native artifact contract
for bindings and examples.

The metadata should include:

- Variant name.
- Target operating system and architecture.
- Render backend and OpenGL context provider.
- Artifact shape: `static-monolithic` or `shared-private`.
- Core library path.
- Windows import library path when applicable.
- Public include directories.
- Private dependency library directories required for final linking or loading.
- Runtime search paths required for development builds.
- System libraries and frameworks required by static final links.
- C ABI version expected by generated or compiled bindings.

The JSON metadata is an internal interchange format. Repository build scripts
should transform it once, in `//:native-metadata`, into the simplest native
input each consumer can use:

- `pkg-config` for consumers with first-class `pkg-config` support.
- Gradle properties for JVM and Kotlin/Native builds.
- MSBuild props for .NET development and test projects.
- Swift linker flags and Zig key/value config files for tools without a direct
  `pkg-config` integration point.

Consumer build systems should parse the JSON metadata only when no simpler
generated format fits their native model.

## Binding Consumption

Bindings and examples should consume generated native inputs instead of
reconstructing library names, include paths, link paths, or runtime search paths
from the build directory.

After Zig consumes the metadata, Rust, Swift, and Kotlin/Native should move to
`static-monolithic` for native final-binary workflows.

Java, .NET, and the consolidated JVM/KMP binding should consume `shared-private`
for runtime-loaded workflows. Their loaders should share the same
exact-core-library override concept, while preserving runtime-specific error
reporting and ABI checks.

Examples should follow the binding they exercise. Example-specific graphics
libraries such as SDL, GLFW, LWJGL, Silk.NET, or platform windowing APIs remain
owned by the example.

## Migration Plan

1. Derive the artifact shape from platform policy, with current shared behavior
   preserved and iOS device builds mapped to static.
2. Generate the variant metadata file from CMake configure output.
3. Replace glob-based native artifact checks with metadata validation.
4. Move Zig C API tests, Zig binding tests, and Zig examples to generated native
   config.
5. Make `static-monolithic` the default for native final-binary consumers.
6. Move Rust, Swift, and Kotlin/Native native workflows to `static-monolithic`.
7. Move runtime-loaded bindings to `shared-private` with strict `mln_*` export
   control.
8. Remove duplicated per-language linker and loader metadata once each binding
   consumes the generated contract through `pkg-config` or generated
   tool-specific inputs.

## Non-Goals

This document does not define public binary packaging, package-manager layouts,
or release artifact naming.

This document does not require one artifact shape for every consumer. Native
final binaries and separate language runtimes have different integration
boundaries.
