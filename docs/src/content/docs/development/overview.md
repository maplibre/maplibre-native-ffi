---
title: Overview
description:
  Set up a development machine, build for other targets, and debug on CI.
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
running. An emulator boots in minutes under KVM on Linux. Without KVM it runs in
software, which takes an hour or more on Linux, but the OpenHarmony emulator
boots in under a minute on an Apple silicon Mac. Every checkout on a machine
shares one Android emulator per API level and ABI: the AVD `api-<level>-<abi>`,
such as `api-36-arm64-v8a`, in the default AVD home. A task reuses the one that
is running, whoever started it, and creates the AVD if it is missing. Since
another session may be using it, no task stops it; run
`mise run //:android-emulator:stop <abi> --api <level>` when none is.

Every checkout also shares the one OpenHarmony emulator that runs the Oniro
image mise installed. It forwards hdc from `127.0.0.1:55555`, and a task reuses
it, whoever started it. Run `mise run //:ohos-emulator:stop` when no session is
using it.

## FreeBSD

The FreeBSD presets cross-compile on a Linux host. `mise run test` runs their
suites in a FreeBSD virtual machine, which it boots on demand with
[anyvm](https://github.com/anyvm-org/anyvm) and leaves running. The machine
needs QEMU, OVMF, and access to `/dev/kvm`; on Ubuntu, install
`qemu-system-x86`, `qemu-utils`, and `ovmf`. Each checkout gets its own machine,
which mounts the checkout at the same path and keeps its disk under
`build/freebsd-vm`. Run `mise run //:freebsd-vm:stop` to shut it down.

## Windows on Linux

The Windows presets also cross-compile on a Linux host, with LLVM's `clang-cl`,
`lld-link`, `llvm-lib`, `llvm-rc`, and `llvm-mt` from `PATH`. The `msvc` mise
environment downloads the MSVC runtime and the Windows SDK from Microsoft with
[xwin](https://github.com/Jake-Shadle/xwin). Installing it accepts
[Microsoft's license](https://go.microsoft.com/fwlink/?LinkId=2086102) for them:

```bash
mise -E msvc install
```

`mise run test` runs the suites under the Wine executable that `WINE` names,
such as `files/bin/wine` in Steam's Proton, with a prefix under
`build/wine-prefix`. An x64 host runs only the x64 presets. Wine implements WGL
over the host's OpenGL and Vulkan over the host's Vulkan driver, so Windows CI
remains the authority on Windows behavior.

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

The testbox shell has the workflow's sccache settings but not its S3
credentials, so builds fail until you prefix the command with
`env -u SCCACHE_BUCKET -u SCCACHE_ENDPOINT -u SCCACHE_REGION -u SCCACHE_S3_USE_SSL`,
which makes sccache use the testbox's local disk.
