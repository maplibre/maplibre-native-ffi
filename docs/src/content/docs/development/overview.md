---
title: Overview
description: Set up a development environment and run the common tasks.
sidebar:
  order: 1
---

This repository holds a C API for MapLibre Native and the language bindings that
wrap it. The C API covers the core of MapLibre Native: runtimes, resources,
maps, cameras, events, diagnostics, and render targets. Convenience features
such as snapshotting, and platform integrations such as gestures, belong to
higher-level SDKs outside this repository.
[Concepts](/maplibre-native-ffi/concepts/) explains the model, and the
[Binding specification](/maplibre-native-ffi/development/binding-specification/)
states what every binding must do.

## Set up

Install the prerequisites for your host:

- **macOS** on Apple Silicon: Homebrew and Xcode 26.0.1.
- **Linux**: nothing beyond mise. Bootstrap installs the development libraries
  with apt on Ubuntu and dnf on Fedora. On other distributions, install the
  packages listed in `mise.linux.toml`.
- **Windows**: run these commands from PowerShell.

  ```powershell
  winget install --exact --id Git.Git
  winget install --exact --id KhronosGroup.VulkanSDK
  winget install --exact --id LLVM.LLVM
  winget install --exact --id Microsoft.VisualStudio.2022.BuildTools --override "--passive --wait --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended --add Microsoft.VisualStudio.Component.VC.Tools.ARM64"
  ```

  Project tasks run in Git Bash.

Install [mise](https://mise.jdx.dev/), then bootstrap the repository. Bootstrap
installs system packages, the shared toolchain, and Git hooks:

```bash
mise trust
mise bootstrap --yes
```

On Windows, add `--skip files` to the bootstrap command.

Check the setup with the headless readback example:

```bash
mise run //examples/zig-readback:run
```

## Common tasks

```bash
mise run build                       # Build the native library for the host
mise run build linux-gnu-x64-egl     # Build another target or render backend
mise run test                        # Build and run the C API tests
mise run //bindings/rust:test        # Run one binding's tests
mise run //examples/zig-map:run      # Run an example
mise run //docs:build                # Build this documentation site
mise run fix                         # Format and lint every file
mise tasks --all                     # List every task
```

The host build uses Metal on macOS and Vulkan on Linux and Windows.
`CMakePresets.json` defines every target and render backend. Bindings, examples,
and the docs declare their own tools, and mise installs them the first time one
of their tasks runs.

## Targets that need an SDK

Android, Emscripten, and OpenHarmony builds need a cross-compilation SDK of
several gigabytes, so mise installs one only on request. Point the build at an
SDK you already have:

| Target         | Environment variable |
| -------------- | -------------------- |
| `android-*`    | `ANDROID_HOME`       |
| `emscripten-*` | `EMSDK`              |
| `ohos-*`       | `OHOS_SDK_NATIVE`    |

Or let mise install the pinned SDK, and pass the same `-E` to every command that
builds for that target:

```bash
mise -E android install
mise -E android run build android-arm64-vulkan
```

Set `MISE_ENV=android,ohos` to select environments for a whole shell.

The x64 Android and OpenHarmony tests run in an emulator that boots on demand
and keeps running until you stop it:

```bash
mise run test android-x64-egl
mise run //:android-emulator:stop
```

Emulators need KVM on Linux. Without it, a boot takes an hour or more.

To prepare the Android SDK before running Gradle or CMake directly, run
`mise run android-sdk-packages`. After changing the pinned NDK version, run
`mise run clean <preset>` for each Android preset that you built before, because
CMake caches the compiler path.

## Where things live

| Concern                                  | Owner                                                 |
| ---------------------------------------- | ----------------------------------------------------- |
| Tools, system packages, tasks, Git hooks | mise: `mise.toml` and each project's `mise.toml`      |
| Native targets and render backends       | CMake presets: `CMakePresets.json`                    |
| Android and Kotlin packages              | Gradle                                                |
| Language dependencies                    | Each ecosystem's package manager, such as uv or Cargo |
| Formatting and lint                      | hk (`hk.pkl`) and dprint                              |
| Compiler cache                           | sccache with a shared read-only cache                 |
| CI workflows                             | `ci/`, generated into `.github/workflows/`            |

Edit the CI configuration in `ci/`, then run `mise run ci:generate-workflow` to
regenerate the workflows. Hygiene fails when the checked-in workflows are out of
date.

## CI coverage

Applicable CI jobs are selected automatically. If the selection looks wrong,
trigger a manual run (`gh workflow run CI --ref <branch>`).

### Debug on a CI runner

Use [Blacksmith Testbox](https://github.com/useblacksmith/begin-testbox) to run
individual commands against local changes on a runner with CI's dependencies.
Authenticate with `blacksmith auth login`; your account needs access to the
MapLibre organization in Blacksmith.

Warm the Linux job from your branch, then use the returned ID to sync local
changes over rsync and run a command:

```bash
blacksmith testbox warmup .github/workflows/blacksmith-testbox.yml --job linux --ref <branch>
blacksmith testbox run --id <id> "mise run test linux-gnu-x64-egl"
```

Replace the test command with `mise run //bindings/<lang>:test <preset>` to test
a binding. The Linux job prepares x64 GNU and musl presets with EGL or Vulkan.
Android/OHOS emulators, Emscripten, and Linux ARM64 need separate target setup
or runners. The `macos` job mirrors CI's Apple setup, but the pinned Testbox
actions use Linux-specific commands; confirm upstream macOS support before using
that job. Warmup prepares dependencies and waits for commands; it runs no tests
itself.

## Tests and examples

Give every feature automated coverage where practical. `mise run test` runs the
C API suite, and each binding has its own test task. Use an example for behavior
that needs a person to check it, such as visual output, input, or host graphics
integration. Keep examples small.
