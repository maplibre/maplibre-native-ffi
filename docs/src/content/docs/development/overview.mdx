---
title: Overview
description: Set up a machine, build and test, and work on other targets.
sidebar:
  order: 1
# Scope: what a maintainer runs or installs by hand to set up, build, test,
# change the engine, use CI, and release. Leave out what the tooling does on its
# own, uncommon cases, and commands that upstream projects document.
---

## Set up

Most development happens on Linux and macOS. CI tests Windows, but the local
Windows setup gets little use and may be broken.

[mise](https://mise.jdx.dev/) manages the toolchain. Install it, then run
`mise trust` and `mise bootstrap --yes`. On Ubuntu and Fedora, bootstrap also
installs the system packages. On other Linux distributions, install the
equivalents of the packages in `.config/mise.linux.toml`. On macOS, install
Xcode 26. On Windows, install these first:

```powershell
winget install --exact --id Git.Git
winget install --exact --id KhronosGroup.VulkanSDK
winget install --exact --id LLVM.LLVM
winget install --exact --id Microsoft.VisualStudio.2022.BuildTools --override "--passive --wait --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended --add Microsoft.VisualStudio.Component.VC.Tools.ARM64"
```

## Everyday commands

`mise run test` builds the native library for this machine and runs its C tests.
`mise run //bindings/<lang>:test` tests one binding against that build. Pass a
preset name to build for another target, such as `linux-gnu-x64-vulkan` or
`macos-arm64-metal`. `CMakePresets.json` lists them.

After you change a header, run `mise run bindings:generate`. `mise run check`
runs the linters, and `mise run fix` fixes what it can; the pre-commit hook does
the same for staged files. `mise run //docs:dev` serves this site.
`mise tasks --all` lists everything else.

## Other targets

### Android, OpenHarmony, and Emscripten

These SDKs are large, so they're opt-in. Install one with
`mise -E android install`, `-E ohos`, or `-E emscripten`, and pass the same `-E`
to every later command, or set `MISE_ENV`. After an NDK update, run
`mise run clean <preset>` for each Android preset.

### Emulators and simulators

Tests that need a device get one from
[devices-cli](https://github.com/sargunv/devices-cli), which mise installs.
`mise run test` boots an emulator or simulator for Android, OpenHarmony, iOS,
tvOS, and visionOS, or a virtual machine for FreeBSD, and leaves it running.
`devices-cli release` stops this worktree's devices. Android emulators need KVM.
To test on your own Android device, set `ANDROID_SERIAL`.

### Windows from Linux

The Windows presets build on Linux. `mise -E msvc install` downloads the MSVC
runtime and the Windows SDK, and accepts
[Microsoft's license](https://go.microsoft.com/fwlink/?LinkId=2086102) for them.
Set `WINE` to a Wine executable, then run tests with
`mise -E msvc run test windows-x64-vulkan`. Wine isn't Windows: when results
differ, Windows CI is right.

### MinGW

On Windows, install [MSYS2](https://www.msys2.org/) and run this in its MINGW64
shell:

```bash
pacman -S --needed mingw-w64-x86_64-clang mingw-w64-x86_64-llvm mingw-w64-x86_64-pkgconf
```

Then run tasks from PowerShell with `-E msys2`, such as
`mise -E msys2 run test mingw-x64-wgl`. Set `MLN_FFI_MSYS2_ROOT` if the MINGW64
prefix isn't `C:/msys64/mingw64`.

### FreeBSD

The FreeBSD presets build on Linux, and their tests run in a virtual machine.
Install QEMU and OVMF, and make sure you can access `/dev/kvm`.

### RISC-V and LoongArch

These presets build on any Linux host, and their tests run under QEMU. Install
Docker, then run `bash scripts/setup-qemu.sh` to install and register the
emulators. Run it again after a reboot.

## Change the engine

Each patch in `patches/maplibre-native/` is a change we also send upstream.
Start it with why it's needed and a link to the upstream issue. Once you open
the upstream pull request, add `Upstream:` and its link.

`mise deps` resets the submodule and applies the patches in filename order. Any
edit in the submodule that isn't in a patch is lost.

To add a patch, commit the patched submodule, make your change, and save the
diff:

```bash
mise deps
git -C third_party/maplibre-native add -A
git -C third_party/maplibre-native commit -m baseline
# edit third_party/maplibre-native
git -C third_party/maplibre-native diff --ignore-submodules \
  > patches/maplibre-native/0067-short-name.patch
# write the header above the diff
mise deps
```

To edit an existing patch:

1. Reset the submodule to its pin, and apply only the patches before it with
   `git apply`.
2. Commit, then apply the patch you're editing.
3. Make your change, and save the diff over the patch.
4. Run `mise deps` to check that the later patches still apply.

Test a patch with upstream's tests in its `test/` tree. Build and run them as
the
[MapLibre Native docs](https://maplibre.org/maplibre-native/docs/book/platforms/linux/index.html)
explain, after you initialize the submodules they need, such as `googletest`,
which `mise deps` skips. CI doesn't run these tests, so list the ones you ran in
the pull request.

## CI

[Rubberstamp](https://github.com/apps/rubberstamp-bot) labels each pull request
and picks which CI units it runs, from `.config/rubberstamp.yml`. `main` runs
every unit after merge. To run a unit that a pull request skipped, start a
manual run and list it by its name under `ci.units`:

```bash
gh workflow run CI --ref <branch> -f 'units=["mingw"]'
```

To debug on a CI runner, use a
[Blacksmith Testbox](https://github.com/useblacksmith/begin-testbox). Your
Blacksmith account needs access to the MapLibre organization. Run these from the
repository root, since the testbox copies the current directory.

```bash
blacksmith auth login
blacksmith testbox warmup .github/workflows/blacksmith-testbox-linux-x64.yml --job linux-x64 --ref <branch>
blacksmith testbox run --id <id> "mise run test linux-gnu-x64-egl"
```

Builds fail on the testbox unless you prefix them with
`env -u SCCACHE_BUCKET -u SCCACHE_ENDPOINT -u SCCACHE_REGION -u SCCACHE_S3_USE_SSL`.

## Releases

`mise run ci:release-version --next` prints the next version. To release, push a
signed, annotated tag on a `main` commit whose CI passed. Its message is
`MapLibre Native FFI <version>`, a blank line, and the release notes. The
`release` agent skill can draft the notes.

Snapshots publish from `main` daily. To publish one now, run the Publish
snapshots workflow. Before a new PyPI distribution's first release, register its
pending trusted publisher on PyPI.
