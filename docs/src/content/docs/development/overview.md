---
title: Overview
description:
  Contributor setup, project scope, workflow commands, tests, and examples.
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
  installs the required Homebrew packages. The macOS, iOS, tvOS, and Mac
  Catalyst presets compile on that host.
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
initial bootstrap stays focused on tools used across the repository.

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
environment:

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

Android native build tasks install the pinned NDK into the selected SDK when it
is missing. Gradle provisions the SDK that its Android plugin selects, including
`sdk.dir` in `local.properties`. Device tests and Linux Maven staging provision
`ANDROID_HOME`. These tasks reuse installed packages on subsequent runs.

To prepare the SDK before invoking Gradle or CMake directly:

```bash
mise run android-sdk-packages
```

Pass `--ndk-only` to prepare an SDK for native compilation alone. Pass
`--sdk-root <path>` to provision a specific SDK.

The repository pins an NDK LTS release in `mise.toml`; runner image updates
affect download cost rather than compiler selection. The Android package
versions are `mise.toml` variables, and the Git-ignored `mise.local.toml` at the
repository root overrides them.

After changing the NDK version, run `mise run clean <preset>` for each Android
preset that you previously built, then `mise run --force build <preset>`. CMake
caches the compiler path across configuration runs.

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

GitHub Actions runs those checks. `ci/workflow.toml` declares the baseline and
ready targets and the suites each target runs. `ci/generate_workflow.py` builds
workflow objects and serializes them as YAML. `mise run ci:generate-workflow`
updates the generated workflows under `.github/workflows/`; `--check` verifies
that the checked-in files match. `ci/snapshots.toml` declares the input scope of
each component the daily snapshot workflow publishes, so a component republishes
only when the paths it consumes changed; `mise run ci:check-snapshot-scopes`
keeps every tracked path classified.

[Astro](https://astro.build/) and [Starlight](https://starlight.astro.build/)
build the documentation site. Generated API reference HTML is installed into
`docs/public/reference/` before each docs build.

## CI coverage

One workflow, `CI` (`ci.yml`), runs every check, and `ci-required` is its one
required status check. Its first job plans the run by choosing coverage units,
and every other job runs only when its unit is selected. Hygiene always runs
when the pull request head changes. `ci/select.toml` defines the other units,
each a group of jobs such as every Windows target, along with a description of
what each unit catches.

Main pushes and manual runs select every unit. For a pull request, the planner
asks [Jev](https://docs.typesafe.ai/) one question per unit: whether a reviewer
would want that unit to run to verify the change. The question carries the diff,
the commit messages, and the descriptions from `ci/select.toml`. A unit runs
when Jev answers yes with a probability of at least 0.5, when a changed file
matches one of the unit's `paths`, or when the pull request has the `ci:<unit>`
label. The `ci:full` label selects every unit.

Each unit judges only the pull request's own changes since the unit last passed
for it. The planner reads the jobs of earlier runs on the same pull request, and
a unit passed in a run when every one of its jobs succeeded there. Changes that
arrive by merging main are left out, because main CI tested them: the planner
merges the earlier commit with the main commit now in the pull request and
compares that merge with the new head. A push that only fixes a formatting nit
after broad coverage therefore reruns hygiene alone, and a push that only merges
main reruns nothing else. A cancelled or failed unit keeps its older base and
runs again once the change puts it at risk.

The planner selects every unit when it cannot ask Jev. That covers fork and
Dependabot pull requests, which receive no `TYPESAFE_API_KEY` secret, any API
failure or timeout, and a change too large for eight requests. Main CI runs
every unit after merge, so a selection that misses a regression surfaces on main
rather than in a release.

The plan job prints each unit's decision and uploads a `ci-plan` artifact that
records the decision, Jev's probability, and the commits judged for every unit,
along with a fingerprint of the questions. Download records with
`gh run download <run-id> -n ci-plan` to inspect past decisions when tuning the
threshold or `ci/select.toml`.

Selecting a packaging job also selects the target jobs that produce its inputs.
All native packages, the Apple XCFramework, and the verified Maven repository
belong to the full main run, and snapshot publishing consumes that run. CI
retries one primary failure once, along with `ci-required`.

`mise run ci:test` exercises unit ownership, planning, the required check,
retries, and release tooling without network access. To evaluate a change to
`ci/select.toml` against history, set `TYPESAFE_API_KEY` and run
`uv run python -m ci.select_backtest` for the labeled commits in
`ci/select_cases.toml`, or add `--range origin/main --since <date>` for every
commit in that range. `uv run python -m ci.select_replay --since <date>` replays
the pushes of merged pull requests and estimates runner time.

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
