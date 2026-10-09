---
title: Overview
description: Set up a development environment, build any target, and run CI and coverage.
sidebar:
  order: 1
---

This page covers machine setup and the parts of the workflow that span the
repository. The root `AGENTS.md` lists the everyday commands and the project's
rules, and each area keeps its own README: `tools/bindgen`, `tests/native`,
`tests/graphics`, and `examples`.

## Set up

Install the platform prerequisites:

- On macOS Apple Silicon, install Homebrew and Xcode 26.0.1. Mise bootstrap
  installs the required Homebrew packages.
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
installs the first time one of their tasks runs. The devcontainer image includes
every tool.

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

`ci/workflow.toml` declares the targets of each tier and the suites that each
target runs. `mise run ci:generate-workflow` writes the workflows under
`.github/workflows/` from it, and `--check` verifies them. CI builds every
preset in `CMakePresets.json` except a local tool, such as the coverage preset,
whose `vendor` settings set `maplibre-native-ffi.ci` to false.

Branch protection requires three checks:

| Check                    | Coverage                                                        |
| ------------------------ | --------------------------------------------------------------- |
| `ci-required (baseline)` | Hygiene, docs, and Linux x64 EGL/Vulkan on every PR code update |
| `ci-required (ready)`    | macOS, Windows x64, Android x64, and browser, once out of draft |
| `ci-required`            | The extended workflow: requested platforms or full verification |

PR labels add platforms to the extended workflow. Labels combine and persist
across pushes:

| Label        | Adds                                                              |
| ------------ | ----------------------------------------------------------------- |
| `ci:apple`   | All macOS backends and the iOS, Mac Catalyst, and tvOS targets    |
| `ci:android` | All Android ABIs and backends, and multi-ABI packaging            |
| `ci:linux`   | Linux ARM64 and musl                                              |
| `ci:windows` | Windows ARM64                                                     |
| `ci:ohos`    | OpenHarmony targets and emulator tests                            |
| `ci:full`    | Every target and complete packaging verification, including Maven |

Main, manual runs, Dependabot PRs, and `ci:full` run every target and packaging
check in one workflow, `ci.yml`. Snapshot publishing consumes a successful main
run. An unrequested tier passes its check without running targets.

A change of draft state or labels reuses earlier results only for the same
tested merge commit and coverage scope. Missing, cancelled, or first-attempt
failed coverage runs again, as does an explicit rerun. `mise run ci:test` tests
these transitions.

Each target job runs every suite even after one fails, and a final step fails
the job. CI reruns a failed run once only when every failed job failed in an
infrastructure step that `ci/retry.py` lists, so a failed test is never retried.

`ci/snapshots.toml` declares the inputs of each component that the daily
snapshot workflow publishes, and `mise run ci:check-snapshot-scopes` keeps every
tracked path classified.

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
