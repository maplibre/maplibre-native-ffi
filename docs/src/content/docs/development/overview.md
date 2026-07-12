---
title: Overview
description: Contributor setup, project scope, workflow commands, tests, and examples.
sidebar:
  order: 1
---

## Project Scope

The project exposes MapLibre Native through two layers.

The C API exposes core MapLibre Native features on supported native platforms:
runtime, resources, maps, cameras, events, diagnostics, logging, render target
primitives, texture readback, and low-level extension points such as resource
providers and URL transforms. It excludes convenience APIs such as snapshotting
and platform integrations such as gestures and device sensors.

Language bindings sit directly above the C API. In the target language, they
manage C handles, struct initialization, scoped lifetimes, status codes,
diagnostics, borrowed data, threading, and event draining. They preserve the C
API's concepts. Higher-level adapters may provide full SDKs, async models, view
lifecycle integrations, convenience workflows, or new abstractions.

Read the
[Binding specification](/maplibre-native-ffi/development/binding-specification/)
before implementing or reviewing a binding.

## Getting Set Up

Install the platform prerequisites:

- On macOS Apple Silicon, install Homebrew and initialize the Xcode version
  listed in `.xcode-version`, or a recent compatible Xcode.
- On Linux, mise bootstrap installs the compiler and development libraries
  through apt on Ubuntu and dnf on Fedora.
- On Windows, install Visual Studio Build Tools 2022 with the
  `Desktop development with C++` workload and C++ Clang tools component, Git for
  Windows, and the Vulkan SDK. We rely on Git Bash to run project scripts.
- For Android, install the Android SDK packages pinned in `mise.toml`.
- For OpenHarmony, install an API 24 SDK.

Install [`mise`](https://mise.jdx.dev/), then bootstrap system packages, install
pinned project tools, and run repository setup hooks:

```bash
mise trust
mise bootstrap
```

Android and OpenHarmony builds require their SDK paths in environment variables.
Put the absolute paths in the Git-ignored `mise.local.toml` at the repository
root:

```toml
[env]
ANDROID_HOME = "/home/you/Android/Sdk"
OHOS_SDK_NATIVE = "/home/you/HarmonyOS/command-line-tools/sdk/default/openharmony/native"
```

Use the Android SDK package versions pinned in `mise.toml`. Override them in
`mise.local.toml` when testing another toolchain version. Only define the
Android variable when building Android targets and `OHOS_SDK_NATIVE` when
building OpenHarmony targets. Mise loads `mise.local.toml` automatically.

The setup hooks install repository hooks, initialize the MapLibre Native
submodule at `third_party/maplibre-native`, and refresh generated support data.
Each workflow installs the package-manager dependencies it owns.

Run the headless Zig readback example as a smoke test:

```bash
mise run //examples/zig-readback:run
```

Host workflows use the matching default CMake preset: Metal on macOS and Vulkan
on Linux and Windows. Pass a preset to the native task to build another native
target or backend:

```bash
mise run build linux-x64-egl
```

Android is Gradle-owned. Its mise tasks default to OpenGL for both supported
ABIs. Pass the backend and ABI set to select another build:

```bash
mise run //examples/android-map:build
mise run //examples/android-map:build vulkan arm64-v8a
```

OpenHarmony is Hvigor-owned. Build either native backend through its platform
project:

```bash
mise run //bindings/openharmony:build ohos-arm64-vulkan
```

The HAR is written under `build/packages/openharmony/<preset>/`.

## Common Commands

```bash
# Build and test the C API
mise run test

# Build only
mise run build

# Run linters and formatters
mise run fix

# Run examples
mise run //examples/zig-map:run:owned-texture

# Build the documentation site
mise run //docs:build
```

## How Tools Fit Together

This repository spans native code, language bindings, examples, tests, and
documentation. Each tool owns the layer where it has the clearest dependency
model.

Mise supplies repository tools, system package declarations, and small common
workflow tasks. Those tasks call the build system that owns each ecosystem and
pass artifacts explicitly. Platform SDKs such as Xcode, Visual Studio, Linux
graphics drivers, and the Android SDK remain host toolchain inputs.

Native dependencies follow four ownership rules:

- System package managers provide Linux compilers and desktop development
  libraries through mise bootstrap.
- Platform SDKs own Apple and Windows compilers, system libraries, graphics
  drivers, frameworks, and mobile toolchains.
- MapLibre Native's pinned submodules own libraries compiled into the native
  artifact.
- CMake acquires pinned Windows zlib and libuv sources and the ANGLE runtime for
  macOS EGL builds.

CMake discovers each provider and exposes project-owned dependency targets to
the platform and renderer modules. Those modules consume targets rather than
package-manager paths, raw library searches, or SDK layout details. CTest uses
the same target metadata for headers, library search paths, runtime paths, and
the Vulkan ICD.

[`mise`](https://mise.jdx.dev/) is the contributor entrypoint. It pins top-level
tools, installs Git hooks, and runs repository tasks. Use `mise run ...` for
common workflows: build, test, check, fix, and examples. Mise delegates native
variant definitions to CMake presets. Gradle and Hvigor select those presets for
Android and OpenHarmony platform builds instead of imposing a repository-wide
target environment.

Mise bootstrap installs Linux and macOS native prerequisites through apt, dnf,
or Homebrew. CMake uses the host compiler and standard package discovery rather
than a repository-local dependency prefix. On Windows, Visual Studio supplies
the toolchain while CMake builds pinned zlib and libuv sources. CMake builds
portable native artifacts and CPack archives desktop distributions. Gradle
invokes Android presets and packages their outputs with the Kotlin binding and
applications. Hvigor invokes OpenHarmony presets and packages their outputs.

Gradle owns Android platform builds and Hvigor owns OpenHarmony platform builds.
Both select native variants defined by CMake presets, then own their platform
module and application packaging. Direct CMake workflows remain available for
native development and diagnostics without duplicating variant definitions.

Language package managers own dependencies inside their ecosystems. For example,
`uv` owns Python package dependencies, `pnpm` owns Node package dependencies,
Gradle owns Java and Kotlin dependencies, and Cargo owns Rust dependencies.
Language-specific formatters, linters, analyzers, test frameworks, and code
generators usually live with the language package graph they serve.

[`hk`](https://github.com/jdx/hk) orchestrates repository checks for pre-commit,
`mise run check`, and `mise run fix`. [`dprint`](https://dprint.dev/) owns
repository-wide formatting defaults.

[Astro](https://astro.build/) and [Starlight](https://starlight.astro.build/)
build the documentation site. Generated API reference HTML is installed into
`docs/public/reference/` before each docs build.

## Tests And Examples

Every feature needs automated CI coverage when practical. The root
`mise run test` command builds the native library and runs the direct Zig C API
suite. Language binding suites run through their binding-specific CI tasks.

Use examples for demos and behavior that needs manual validation, such as visual
output, interactive input, or host graphics integration.

Keep examples small. This repository includes low-level language bindings and
focused integration examples. Full application SDKs live outside this
repository.
