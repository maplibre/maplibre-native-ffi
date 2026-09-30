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
[Binding generation](/maplibre-native-ffi/development/binding-generation/)
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

GitHub Actions runs those checks. `ci/workflow.toml` declares the baseline and
ready targets and the suites each target runs. CI builds every preset in
`CMakePresets.json` except a local tool, such as the coverage preset, whose
`vendor` settings set `maplibre-native-ffi.ci` to false.
`ci/generate_workflow.py` builds workflow objects and serializes them as YAML.
`mise run ci:generate-workflow` updates the generated callers and reusable
workflows under `.github/workflows/`; `--check` verifies that the checked-in
files match. `ci/snapshots.toml` declares the input scope of each component the
daily snapshot workflow publishes, so a component republishes only when the
paths it consumes changed; `mise run ci:check-snapshot-scopes` keeps every
tracked path classified.

[Astro](https://astro.build/) and [Starlight](https://starlight.astro.build/)
build the documentation site. Generated API reference HTML is installed into
`docs/public/reference/` before each docs build.

## CI coverage

CI has three required checks:

| Check                    | Coverage                                                          |
| ------------------------ | ----------------------------------------------------------------- |
| `ci-required (baseline)` | Hygiene, docs, and Linux x64 EGL/Vulkan on every PR code update   |
| `ci-required (ready)`    | Additional representative targets when a PR leaves draft status   |
| `ci-required`            | Requested platforms or full verification in the extended workflow |

Promotion starts ready coverage while baseline results remain valid. The
extended workflow combines platform labels into one selection. It builds each
selected target once and includes every producer needed by Android multi-ABI
packaging. Extended coverage can repeat targets covered by baseline or ready CI.

State changes reuse actual success, or a failure after its one retry, only for
the same tested merge commit and complete coverage scope. Adding or removing a
platform changes the scope; unrelated labels preserve it. Omitted checks and
restated verdicts never prove that tests ran. Missing or cancelled coverage
executes again, as does an explicit workflow rerun. An attempt-1 failure is also
missing evidence, because CI retry may replace it. Selected jobs must succeed;
only unselected jobs may be skipped.

The extended workflow, `CI` (`ci.yml`), runs every target and complete packaging
verification on main, manual runs, Dependabot PRs, and PRs with `ci:full`. All
native packages and the verified Maven repository belong to that run. Snapshot
publishing consumes that single successful main run. Main and manual runs have
independent concurrency groups; a new PR commit cancels obsolete work in each PR
workflow.

Branch protection must require all three checks. Add baseline and ready
alongside the existing `ci-required` before merging these workflows. An
unrequested tier passes its check without running targets.

`mise run ci:test` exercises coverage transitions, result reuse, generated job
dependencies, required checks, retries, and release tooling.

Each target job builds the native library in its own step, then runs the C suite
and every binding suite. A suite runs even after an earlier suite failed, as
long as the build and any device boot succeeded, so one run reports every failed
suite. A final step fails the job when any suite failed.

CI reruns a failed run once, with its dependent verification and required
checks, only when every failed job failed in an infrastructure step: runner
setup, checkout, coverage planning, `setup-ci-deps`, SwiftPM resolution, or a
device boot. `ci/retry.py` lists those step names, and a failed test is never
retried.

## Tests And Examples

Every feature needs automated CI coverage when practical. The root
`mise run test` command builds the native library and runs the native C suites
in `tests/native` through CTest and Unity. The ABI suite links the shipped
library through its public headers. The internal suite links the static library
and uses the library's sync points to order threads that no public fence can
order. On targets that run through CTest, each program in `tests/native/exit` is
a test of its own whose process exit is the check. `tests/native/README.md`
describes the suites and the exit programs. On desktop, simulator, and
Emscripten targets, `mise run test` also runs the host unit tests of the Rust
platform crate in `src/platform/rust`, which cover its pure helpers such as
redirect resolution. `mise run build` runs `mise run check-exports`, which fails
when the installed library exports anything beyond the public C API.

Native behavior is tested once, in those C suites. Each binding suite covers
what its binding adds to the C API: the handwritten runtime, the generated
shapes, and the platform integration. `tests/conformance/cases.toml` lists the
cases that every binding covers, and each binding's file beside it maps every
case to a test or to the reason that it does not apply. The `conformance` check
in hk runs `scripts/check-conformance.py --strict`, which fails on an unmapped
case, on a test name that its file does not contain, and on a case still marked
todo. A binding suite runs through its own task, such as
`mise run //bindings/rust:test [preset]`. `mise run bindings:test-generator`
tests the binding generator;
[Generate bindings](/maplibre-native-ffi/development/binding-generation/#test-the-generator)
describes that suite.

Tests wait on signals rather than elapsed time, and serve every request from a
local fixture: a resource provider, a file, or the native suite's loopback HTTP
server on 127.0.0.1. The `test-hygiene` check in hk runs
`scripts/check-test-hygiene.py`, which fails on a sleep or on a public or
reserved host in test code. Its baseline, `scripts/test-hygiene-baseline.toml`,
counts the violations that predate the check, and a count may only fall. The
`export-calls` check runs `mise run check-export-calls`, which fails when the
ABI suite leaves an exported function uncalled by name, and calls from the
internal suite do not count. Its baseline, `tests/uncalled-exports.txt`, also
only shrinks.

A test that cannot run on a target is skipped by that target or its build, never
by what the environment provides. Most suites leave such a test out when they
are built; a few mark it skipped for the platform or backend they were built
for. Rendering tests run on every target that can render, and a CI runner that
lacks a renderer gets one rather than a skip.

Tests take their GPU objects from `tests/graphics`, a small C library that
creates a device or context, a borrowed texture, and a presentation surface for
Metal, Vulkan, EGL, and WGL. The C suite links it, and `mise run build` installs
it as `mln_test_graphics` beside the native library for the binding suites to
load over their FFI. Its README lists how each binding loads it.

The log callback, the async log mask, and the network status are process-global.
A test that changes one runs in a group that no other test runs alongside, and
restores the default when it ends, whether it passed or failed.

Use examples for demos and behavior that needs manual validation, such as visual
output, interactive input, or host graphics integration. CI runs each example's
`smoke` task on every target whose backend the runner can render. The task runs
the example in
[smoke mode](/maplibre-native-ffi/development/map-example-specification/#smoke-mode),
which renders one frame of an inline style where no one sees it and exits. Linux
runners have no display server, so an example that opens a window runs there
under Xvfb or its toolkit's offscreen platform, drawing with Mesa's software
drivers.

Keep examples small. This repository includes low-level language bindings and
focused integration examples. Full application SDKs live outside this
repository.

## Code coverage

Code coverage is a local tool for deciding which suite a test belongs in. CI
runs no coverage build and enforces no coverage percentage.

`mise run coverage [suite]` builds the `macos-arm64-metal-coverage` preset, runs
one suite against it, and writes `build/coverage/<suite>/lcov.info` and
`build/coverage/<suite>/html/index.html`. The suite is `native` for the C tests,
which is the default, or a binding name such as `rust`. The preset instruments
the project's own sources with clang source-based coverage, so the report covers
`src/` alone and leaves out MapLibre Native and the test sources. The preset
builds on macOS, and the task uses the LLVM tools that ship with Xcode. Each run
executes the suite again, past Go's test cache and Gradle's up-to-date checks.

Before you delete a binding test, compare that binding's report with the C
suite's report:

```bash
mise run coverage native
mise run coverage rust
mise run coverage-diff --only-in rust --not-in native
```

`coverage-diff` lists the `src/` lines that the first report runs and the second
misses, grouped by file and function. It exits with status 1 when it lists any
line. Each listed line needs a C test, or a reason that the C suite cannot reach
it on that preset. Coverage records which lines ran and nothing about what a
test asserted, so check that a C test asserts the behavior that the binding test
checked.

`mise run check-export-calls` runs with the other repository checks. It fails
when a function that a public header declares with `MLN_API` appears by name in
no source of the C ABI suite, which is `tests/native` outside `internal/`. The
internal suite links the static library, so its calls prove nothing about the
export. A name inside a macro counts only when a test uses that macro.
`tests/uncalled-exports.txt` lists the functions that no ABI test calls yet. The
check also fails when a listed function is called, so the list only shrinks;
`mise run check-export-calls --prune` removes those names.
