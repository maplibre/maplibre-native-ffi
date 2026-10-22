---
title: Overview
description:
  Set up a development machine, build for other targets, debug on CI, and use
  code coverage.
sidebar:
  order: 1
---

## Set up

Install the platform prerequisites:

- On macOS Apple Silicon, install Homebrew and Xcode 26.0.1.
- On Linux, mise bootstrap installs the development libraries on Ubuntu and
  Fedora. On other distributions, install the packages that `mise.linux.toml`
  lists.
- On Windows, run these commands from PowerShell. Project tasks run in Git Bash.

```powershell
winget install --exact --id Git.Git
winget install --exact --id KhronosGroup.VulkanSDK
winget install --exact --id LLVM.LLVM
winget install --exact --id Microsoft.VisualStudio.2022.BuildTools --override "--passive --wait --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended --add Microsoft.VisualStudio.Component.VC.Tools.ARM64"
```

Install [`mise`](https://mise.jdx.dev/), then run `mise trust` and
`mise bootstrap --yes`. On Windows, add `--skip files`.

## Cross-compilation SDKs

The Android, Emscripten, and OpenHarmony SDKs are several gigabytes each, so
mise installs one only on request, under the mise environment of the same name:

```bash
mise -E android install
```

Pass the same `-E` to every later command for that target, or set `MISE_ENV` for
the shell. After an NDK change, run `mise run clean <preset>` for each Android
preset, because CMake caches the compiler path.

`mise run test` boots an Android or OpenHarmony emulator on demand and leaves it
running. An emulator boots in minutes under KVM on Linux, and takes an hour or
more in software everywhere else.

## Debug on a CI runner

[Blacksmith Testbox](https://github.com/useblacksmith/begin-testbox) runs
commands against local changes on a Linux runner with CI's dependencies. Your
Blacksmith account needs access to the MapLibre organization.

```bash
blacksmith auth login
blacksmith testbox warmup .github/workflows/blacksmith-testbox.yml --job linux --ref <branch>
blacksmith testbox run --id <id> "mise run test linux-gnu-x64-egl"
```

Run `blacksmith testbox run` from the repository root, because rsync mirrors the
current directory with `--delete`.

## Code coverage

`mise run coverage [suite]` builds the `macos-arm64-metal-coverage` preset, runs
one suite, and writes an lcov and HTML report under `build/coverage/<suite>/`.
The suite is `native`, the default, or a binding name. CI runs no coverage.

Before you delete a binding test, compare that binding's report with the C
suite's:

```bash
mise run coverage native
mise run coverage rust
mise run coverage-diff --only-in rust --not-in native
```

Each line that `coverage-diff` lists needs a C test, or a reason that the C
suite cannot reach it. Coverage records which lines ran, not what a test
asserted, so check that the C test asserts what the binding test checked.
