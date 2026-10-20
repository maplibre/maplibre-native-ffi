---
title: Overview
description:
  Set up a development environment, run the common tasks, and work with CI.
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

Each pull request push runs only the coverage that the change puts at risk.
[Jev](https://docs.typesafe.ai/) reads the diff and decides which coverage units
to run, such as the Windows targets or the docs build. A unit that already
passed on the pull request runs again only if the newer changes put it at risk.
Hygiene runs on every push, and `ci-required` is the one required check.

To force a unit, add its `ci:<unit>` label, such as `ci:windows`. Adding or
removing a label on a closed pull request runs no jobs. `ci:full` selects every
unit. Main pushes, manual runs, and pull requests from forks or Dependabot also
run every unit, and so does any planning failure. Because main runs every unit
after merge, a missed selection surfaces there.

`ci/select.toml` defines the units and the description of CI that Jev reads. The
plan job's log shows each unit's decision and the full prompt sent to Jev.
Before changing `ci/select.toml`, evaluate the change against history with
`uv run python -m ci.select_eval commits`, which compares answers with the
labeled commits in `ci/select_cases.toml`. `ci.select_eval prs` replays merged
pull requests and estimates runner time. Both need `TYPESAFE_API_KEY`. To call
Jev through a proxy such as LiteLLM's TypeSafe pass-through, set
`TYPESAFE_BASE_URL` to the proxy's TypeSafe URL and `TYPESAFE_API_KEY` to the
proxy's key.

## Labels

When an issue or a pull request opens, the Label workflow asks Jev which labels
from the scheme in AGENTS.md it needs. Jev reads the title, the description, and
for a pull request the first chunk of the change description that coverage
selection reads. The workflow adds labels only to groups that have none yet. A
kind or an area from an issue template, from Dependabot, or from the author
therefore stays as set. The workflow runs once per item, and it never removes a
label, so later edits by maintainers are final. The job summary lists each
label's score.

`ci/labels.toml` defines the groups, each label's meaning, and the thresholds.
Before changing it, run `uv run python -m ci.label_eval`, which judges the
hand-labeled items in `ci/label_cases.toml` as if they had just opened and
reports misses and extra labels at each threshold.

## Tests and examples

Give every feature automated coverage where practical. `mise run test` runs the
C API suite, and each binding has its own test task. Use an example for behavior
that needs a person to check it, such as visual output, input, or host graphics
integration. Keep examples small.
