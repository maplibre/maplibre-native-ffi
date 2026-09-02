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

- On macOS Apple Silicon, install Homebrew and Xcode 26.0.1. Mise bootstrap
  installs the required Homebrew packages.
- On Linux, mise bootstrap installs the development libraries through apt on
  Ubuntu and dnf on Fedora. On other distributions, install the packages
  analogous to those listed in `mise.linux.toml`. The Linux presets compile with
  `zig cc`, which mise installs, so the distribution compiler builds only the
  tooling around them; see `cmake/toolchains/zig-linux.cmake`.

On Windows, run these commands from PowerShell:

```powershell
winget install --exact --id Git.Git
winget install --exact --id KhronosGroup.VulkanSDK
winget install --exact --id LLVM.LLVM
winget install --exact --id Microsoft.VisualStudio.2022.BuildTools --override "--passive --wait --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended --add Microsoft.VisualStudio.Component.VC.Tools.ARM64"
```

The Visual Studio command installs the Desktop development with C++ workload,
the recommended x64 tools and Windows SDK, and the ARM64 build tools. Project
tasks run in Git Bash.

Install [`mise`](https://mise.jdx.dev/), then bootstrap system packages, install
the pinned shared toolchain, and run repository setup hooks:

```bash
mise trust
mise bootstrap --yes
```

On Windows, skip mise's unused Unix-only managed-files phase:

```powershell
mise bootstrap --yes --skip files
```

Language-specific tools are declared by their binding, example, or docs project.
Mise installs them automatically when a namespaced project task runs, so the
initial bootstrap stays focused on tools used across the repository. The
published devcontainer image bakes the complete tool union for fast startup.

Run the headless Zig readback example:

```bash
mise run //examples/zig-readback:run
```

The default host preset uses Metal on macOS and Vulkan on Linux and Windows.
Pass another preset to select a different native target or backend:

```bash
mise run build linux-gnu-x64-egl
```

## Cross-Compilation SDKs

The Android, Emscripten, and OpenHarmony targets each build against a
cross-compilation SDK. Every one is several gigabytes, so mise installs them on
request rather than during the bootstrap. Each build reads the SDK path from the
environment, and a machine that already carries an SDK is ready as it stands:

| Target         | Environment variable |
| -------------- | -------------------- |
| `android-*`    | `ANDROID_HOME`       |
| `emscripten-*` | `EMSDK`              |
| `ohos-*`       | `OHOS_SDK_NATIVE`    |

To have mise pin one instead, install it under the configuration environment
named after the presets it serves:

```bash
mise -E android install
mise -E emscripten install
mise -E ohos install
```

An environment selects a `mise.<name>.toml` at the repository root, and that
file is where the SDK is declared. Later commands that build for the target take
the same `-E`, and exporting the variable covers a whole shell:

```bash
export MISE_ENV=android,ohos
```

A build for a target whose SDK is missing reports both ways to supply it.

An Android SDK that mise does not own still needs the pinned NDK and CMake
packages, which the `android-*` presets name:

```bash
mise run android-sdk-packages
```

The Android package versions are `mise.toml` variables, and the Git-ignored
`mise.local.toml` at the repository root overrides them.

The Android emulator and its system image are Android SDK packages rather than
mise tools. `//:android-emulator:boot` installs them into `ANDROID_HOME` the
first time it runs. Both emulators boot on demand and keep running until
stopped:

```bash
mise run test android-x64-egl
mise run //:android-emulator:stop

mise run test ohos-x64-egl
mise run //:ohos-emulator:stop
```

Both emulators take their hardware acceleration from KVM on Linux. A host whose
user can read and write `/dev/kvm` boots one in a few minutes. Every other host
runs the guest in software, where a boot takes an hour or more.

## Compiler Cache

Native builds use [`sccache`](https://github.com/mozilla/sccache) through mise.
`mise.toml` pins the tool and sets the public read-only R2 backend plus CMake
compiler-launcher env, so `mise run build` and other mise tasks pick up the
shared cache automatically. CI overrides those settings with write credentials
when available.

## Common Commands

```bash
# Build and test the C API
mise run test

# Build only
mise run build

# Run linters and formatters
mise run fix

# Run examples
mise run //examples/zig-map:run

# Build the documentation site
mise run //docs:build
```

## How Tools Fit Together

This repository spans native code, language bindings, examples, tests, and
documentation. Each tool owns the layer where it has the clearest dependency
model. Xcode and Visual Studio are host toolchain inputs.

[`mise`](https://mise.jdx.dev/) is the contributor entrypoint. It pins shared
and project-specific tools, installs system packages and Git hooks, and runs
repository tasks. Root configuration owns tools used across the repository;
bindings, examples, and docs declare additional tools in their own `mise.toml`
files. Root configuration also pins the cross-compilation SDKs, one per
configuration environment, so an environment that builds fewer targets installs
fewer SDKs. CMake presets define native targets and render backends. CMake uses
platform SDKs and system libraries where available, and acquires pinned native
libraries that are not available from system package managers. Gradle selects
CMake presets and packages Android applications.

Native installs and CPack archives carry the notices for redistributed
dependencies under `share/maplibre-native-c/licenses`. CMake collects notice
files from the selected platform and render targets, and generates Rust
dependency notices from the locked Cargo graph.

Language package managers own dependencies inside their ecosystems. For example,
`uv` owns Python package dependencies, `pnpm` owns Node package dependencies,
Gradle owns Java and Kotlin dependencies, and Cargo owns Rust dependencies.
Language-specific formatters, linters, analyzers, test frameworks, and code
generators usually live with the language package graph they serve.

[`hk`](https://github.com/jdx/hk) orchestrates repository checks for pre-commit,
`mise run check`, and `mise run fix`. [`dprint`](https://dprint.dev/) owns
repository-wide formatting defaults.

GitHub Actions runs those checks, configured from two files under `ci/`.
`ci/workflow.toml` declares the suites each target runs, and
`mise run ci:generate-workflow` renders them into `.github/workflows/ci.yml`.
`ci/snapshots.toml` declares the input scope of each component the daily
snapshot workflow publishes, so a component republishes only when the paths it
consumes changed; `mise run ci:check-snapshot-scopes` keeps every tracked path
classified.

Third-party GitHub Actions pins live in `.github/workflows/action-pins.yml`.
`mise run ci:check-action-pins` verifies that every workflow and composite
action agrees with that catalog, and `mise run ci:fix-action-pins` copies
catalog pins onto consumers. On a Dependabot pull request from this repository,
the hygiene job runs `mise run fix` and commits the result.

[Astro](https://astro.build/) and [Starlight](https://starlight.astro.build/)
build the documentation site. Generated API reference HTML is installed into
`docs/public/reference/` before each docs build.

## Tests And Examples

Every feature needs automated CI coverage when practical. The root
`mise run test` command builds the native library and runs the direct C API
suite through CTest and Unity. Language binding suites run through their
binding-specific CI tasks.

Use examples for demos and behavior that needs manual validation, such as visual
output, interactive input, or host graphics integration.

Keep examples small. This repository includes low-level language bindings and
focused integration examples. Full application SDKs live outside this
repository.
