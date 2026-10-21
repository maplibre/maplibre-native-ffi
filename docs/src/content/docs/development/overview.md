---
title: Overview
description:
  Set up a development environment, build any target, and run the common tasks.
sidebar:
  order: 1
---

This page covers machine setup and the parts of the workflow that span the
repository.
[AGENTS.md](https://github.com/maplibre/maplibre-native-ffi/blob/main/AGENTS.md)
lists everyday commands and project rules. Area READMEs cover the
[binding generator](https://github.com/maplibre/maplibre-native-ffi/blob/main/tools/bindgen/README.md),
[native tests](https://github.com/maplibre/maplibre-native-ffi/blob/main/tests/native/README.md),
[graphics fixtures](https://github.com/maplibre/maplibre-native-ffi/blob/main/tests/graphics/README.md),
and
[examples](https://github.com/maplibre/maplibre-native-ffi/blob/main/examples/README.md).

## Set up

Install the platform prerequisites:

- On macOS Apple Silicon, install Homebrew and Xcode 26.0.1. Mise bootstrap
  installs the required Homebrew packages. The macOS, iOS, tvOS, and Mac
  Catalyst presets compile on that host.
- On Linux, mise bootstrap installs the development libraries through apt on
  Ubuntu and dnf on Fedora. On other distributions, install the packages that
  `mise.linux.toml` lists. The Linux presets compile with `zig cc`, which mise
  installs, and target glibc 2.17; see `cmake/toolchains/zig-linux.cmake`.

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

Install [`mise`](https://mise.jdx.dev/), then install system packages, the
shared toolchain, and the Git hooks:

```bash
mise trust
mise bootstrap --yes
```

On Windows, skip the Unix-only managed-files phase:

```powershell
mise bootstrap --yes --skip files
```

Each binding, example, and the docs site declare their own tools, which mise
installs the first time one of their tasks runs.

`mise run build` builds the host's default preset: Metal on macOS, Vulkan on
Linux and Windows. Pass a preset from `CMakePresets.json` to build another
target or backend. The build installs a prefix into `build/<preset>/install`,
and `mise run package-native <preset>` archives it:

```bash
mise run build linux-gnu-x64-egl
mise run package-native linux-gnu-x64-egl
```

## Cross-compilation SDKs

The Android, Emscripten, and OpenHarmony targets each build against an SDK of
several gigabytes, so mise installs one only on request. A build reads the SDK
path from the environment:

| Target         | Environment variable |
| -------------- | -------------------- |
| `android-*`    | `ANDROID_HOME`       |
| `emscripten-*` | `EMSDK`              |
| `ohos-*`       | `OHOS_SDK_NATIVE`    |

To use the pinned SDK instead, install it under the mise configuration
environment that is named after its presets:

```bash
mise -E android install
mise -E emscripten install
mise -E ohos install
```

Each environment selects a `mise.<name>.toml` at the repository root. Pass the
same `-E` to every later command that builds for the target, or set it for a
whole shell:

```bash
export MISE_ENV=android,ohos
```

Android native builds install the pinned NDK into the selected SDK when it is
missing. To prepare the SDK before you run Gradle or CMake directly, run
`mise run android-sdk-packages`. Pass `--ndk-only` for native compilation alone,
or `--sdk-root <path>` to provision a specific SDK. The package versions are
variables in `mise.toml`, and a Git-ignored `mise.local.toml` overrides them.
After an NDK change, run `mise run clean <preset>` for each Android preset that
you built, because CMake caches the compiler path.

`mise run test` boots an Android or OpenHarmony emulator on demand, and the
emulator keeps running until you stop it:

```bash
mise run test android-x64-egl
mise run //:android-emulator:stop
```

The Android emulator takes the first free console port from 5554 and runs beside
emulators that other checkouts or projects started. The stop task stops only the
emulator that this checkout booted. To run the Android tasks on another device,
set `ANDROID_SERIAL` to its adb serial.

Both emulators use KVM on Linux when the user can read and write `/dev/kvm`, and
boot in a few minutes. On every other host the guest runs in software, and a
boot takes an hour or more.

## Compiler cache

Native builds use [`sccache`](https://github.com/mozilla/sccache), which
`mise.toml` sets as the CMake compiler launcher.

Locally, every checkout of the repository shares one disk cache.
`mise run build` lists each Git worktree in `SCCACHE_BASEDIRS`, so a new
worktree reuses what its siblings compiled. The build turns off sccache's
preprocessor cache mode, because its hits restore depfiles that name another
worktree's headers, and Ninja then misses header edits in the current one.

CI uses an R2 backend that `.github/actions/setup-ci-deps` configures. Push runs
write to it, and pull request runs read it through the public endpoint.

## CI coverage

Rubberstamp plans which coverage units each pull request runs from the units in
`.config/rubberstamp.yml`, and `ci/workflow.toml` maps each unit to its jobs.
The plan job's summary links to the Rubberstamp dashboard with the reason for
each unit. Hygiene always runs, and pushes to `main` and manual runs run every
unit. If the plan looks wrong, trigger a manual run
(`gh workflow run CI --ref <branch>`).

Each target job runs every suite even after one fails, and a final step fails
the job. CI retries a failed run once when every failed job failed in an
infrastructure step that `ci/retry.py` lists.

`ci/snapshots.toml` declares the inputs of each component that the daily
snapshot workflow publishes, and `mise run ci:check-snapshot-scopes` keeps every
tracked path classified.

### Debug on a CI runner

Use [Blacksmith Testbox](https://github.com/useblacksmith/begin-testbox) to run
individual commands against local changes on a runner with CI's dependencies.
Authenticate with `blacksmith auth login`; your account needs access to the
MapLibre organization in Blacksmith.

Warm the Linux job from your branch, then use the returned ID to sync local
changes over rsync and run a command from the repository root:

```bash
blacksmith testbox warmup .github/workflows/blacksmith-testbox.yml --job linux --ref <branch>
cd "$(git rev-parse --show-toplevel)"
blacksmith testbox run --id <id> "mise run test linux-gnu-x64-egl"
```

Replace the test command with `mise run //bindings/<lang>:test <preset>` to test
a binding. The Linux job prepares x64 GNU and musl presets with EGL or Vulkan.
Android/OHOS emulators, Emscripten, and Linux ARM64 need separate target setup
or runners. macOS needs upstream support for the Linux-specific commands in the
pinned Testbox actions. Warmup prepares dependencies and waits for commands; it
runs no tests itself.

Run `blacksmith testbox run` from the repository root: rsync mirrors the current
directory with `--delete`, which removes remote files absent from that
directory.

## Code coverage

Code coverage is a local tool for deciding which suite a test belongs in. CI
runs no coverage build and enforces no coverage percentage.

`mise run coverage [suite]` builds the `macos-arm64-metal-coverage` preset, runs
one suite against it, and writes `build/coverage/<suite>/lcov.info` and
`build/coverage/<suite>/html/index.html`. The suite is `native` for the C tests,
which is the default, or a binding name such as `rust`. The preset instruments
`src/` alone with clang source-based coverage. It builds on macOS only, and uses
the LLVM tools that ship with Xcode. Each run executes the suite again, past
Go's test cache and Gradle's up-to-date checks.

Before you delete a binding test, compare that binding's report with the C
suite's report:

```bash
mise run coverage native
mise run coverage rust
mise run coverage-diff --only-in rust --not-in native
```

`coverage-diff` lists the `src/` lines that the first report runs and the second
misses, grouped by file and function, and exits with status 1 when it lists any
line. Each listed line needs a C test, or a reason that the C suite cannot reach
it on that preset. Coverage records which lines ran and nothing about what a
test asserted, so check that a C test asserts what the binding test checked.
