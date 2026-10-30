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
  Fedora. On other distributions, install the packages that
  `.config/mise.linux.toml` lists.
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

The Swift Android tasks install the matching Swift SDK on demand. They need the
open-source Linux host compiler at the version pinned by mise. ARMv7, ARM64, and
x64 are supported:

```bash
mise -E android run //bindings/swift:build android-arm64-egl
mise -E android run //bindings/swift:test android-x64-egl
```

`mise run test` boots an Android or OpenHarmony emulator on demand and leaves it
running. An emulator boots in minutes under KVM on Linux. Without KVM it runs in
software, which takes an hour or more on Linux, but the OpenHarmony emulator
boots in under a minute on an Apple silicon Mac.

Device tests get a device from
[devices-cli](https://github.com/sargunv/devices-cli), which gives each worktree
its own Android emulator for the ABI and API level they need, its own iOS, tvOS,
or visionOS simulator on the newest installed runtime, or its own OpenHarmony
emulator on the Oniro image mise installed; run `devices-cli release` to stop
the worktree's devices. Set `ANDROID_SERIAL` to use your own Android device
instead, which must run the same ABI and API level.

## FreeBSD

The FreeBSD presets cross-compile on a Linux host. `mise run test` runs their
suites in the worktree's FreeBSD virtual machine from devices-cli, which boots
it with [anyvm](https://github.com/anyvm-org/anyvm) and mounts the worktree at
the same path. The machine needs QEMU, OVMF, and access to `/dev/kvm`; on
Ubuntu, install `qemu-system-x86`, `qemu-utils`, and `ovmf`. It boots from a
snapshot, so what it installs goes away when `devices-cli release` stops it.

## RISC-V and LoongArch

The Linux RISC-V and LoongArch presets cross-compile on any Linux host. Their
test harness links Debian's graphics loaders for the target architecture, which
the first configure downloads into `build/sysroot` with Docker or Podman.
`mise run test` runs their suites under QEMU's user-mode emulator, against a
root of Debian or Alpine packages that it lays out in `build/roots`. Ubuntu and
Fedora package the emulator as `qemu-user`.

Suites that start child processes, including TypeScript, need registered
emulators. Install and register the emulators CI uses before running them;
Ubuntu 24.04's older QEMU corrupts Node's base64 decoding on LoongArch:

```bash
bash scripts/setup-qemu.sh
```

## MSYS2

On Windows x64, install [MSYS2](https://www.msys2.org/) and run this in its
MINGW64 shell:

```bash
pacman -S --needed mingw-w64-x86_64-clang mingw-w64-x86_64-llvm
```

Then, from PowerShell, build and test the C library with:

```powershell
mise -E msys2 run test mingw-x64-wgl
mise -E msys2 run test mingw-x64-vulkan
mise -E msys2 run //bindings/rust:test mingw-x64-wgl
mise -E msys2 run //bindings/rust:test mingw-x64-vulkan
mise -E msys2 run //bindings/zig:test mingw-x64-wgl
mise -E msys2 run //bindings/zig:test mingw-x64-vulkan
```

Set `MLN_FFI_MSYS2_ROOT` if the MINGW64 prefix is somewhere other than
`C:/msys64/mingw64`. The Vulkan tests also need the Vulkan SDK from setup.
Applications loading the DLL need the MINGW64 `bin` directory on `PATH` for its
C++ runtime. Rust uses the matching `x86_64-pc-windows-gnu` target, and Zig uses
`x86_64-windows-gnu`. Kotlin/Native's older MinGW sysroot and C++ runtime are a
separate compatibility requirement.

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

## Run selected CI units

Manual CI runs accept a JSON array of units from `ci.units` in
`.config/rubberstamp.yml`. For example, run the MSYS2 C suites on a branch:

```sh
gh workflow run CI --ref <branch> -f 'units=["mingw"]'
```

Leave `units` unset to run every unit. Hygiene runs with every selection.

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
