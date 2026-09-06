# AGENTS.md

This project is a C API for MapLibre Native, built for low-level language
bindings and host integrations that need a C boundary instead of direct C++
interop or the popular MapLibre Android/iOS SDKs.

## Project map

- `include/` — Public C API headers (the stable ABI surface).
- `src/` — C++ implementation behind the C headers, plus render backend adapters
  (Vulkan, Metal, OpenGL) and the Zig test support shim.
- `bindings/` — Language bindings (Kotlin, Rust, Swift, Zig, .NET, Python, Go,
  Dart) that wrap the C API in idiomatic target-language interfaces.
- `examples/` — Small demo apps per language/backend (`c-map`, `zig-map`,
  `rust-map`, `zig-readback`, `lwjgl-map`, `swift-map`).
- `third_party/` — Vendored dependencies, primarily the MapLibre Native git
  submodule.
- `docs/` — Astro/Starlight documentation site and generated API reference.

## Workflow

```bash
# Install/refresh system packages, shared tools, and repository hooks.
# On Linux this uses sudo; --yes accepts package-manager prompts. Use this on
# Linux and macOS:
mise bootstrap --yes
# On Windows, skip mise's unused Unix-only managed-files phase.
mise bootstrap --yes --skip files

# Project-specific tools install automatically with namespaced tasks.

# List available tasks across the workspace
mise tasks --all

# Configure, build, and install the host native library
mise run build

# Build and run C API tests (also runs build)
mise run test

# Build and run Rust binding tests (also runs build)
mise run //bindings/rust:test

# Headless smoke test — no display needed
mise run //examples/zig-readback:run

# GUI map app — use a brief timeout or run in background
mise run //examples/zig-map:run

# Build a different native target/backend
mise run build linux-gnu-x64-egl

# Package a native artifact with CPack
mise run package-native linux-gnu-x64-egl

# Build the Android binding for one ABI/backend
mise run //bindings/kotlin:android-build opengl x86_64

# Run formatters and linters on _all_ files (will stage affected files)
mise run fix

# Run formatters and linters on targeted files (will stage affected files)
hk fix [FILES...]
```

Native targets and render backends are defined in `CMakePresets.json`. Gradle
selects the Android presets when building platform packages; OpenHarmony and
host workflows use the presets directly.

Clangd uses the compilation database selected by the last
`mise run build <preset>` invocation; select the matching preset before trusting
target-specific diagnostics.

The Android, Emscripten, and OpenHarmony SDKs are opt-in, each behind the mise
configuration environment named after its presets. A build for one of those
targets reads `ANDROID_HOME`, `EMSDK`, or `OHOS_SDK_NATIVE` from the
environment. To use a pinned SDK instead, pass `-E` to the install and to every
later command that builds for the target:

```bash
mise -E android install
mise -E android run build android-arm64-vulkan
```

Formatters and linters run automatically on pre-commit; you usually don't need
to run them manually.

The environment is managed by mise. If you need to run a command that's not
already a mise task, use `mise exec -- ...` so repository tools and dependency
paths are available.

## Pull requests

When you open a pull request, follow the repository PR template and write
**Summary** and **Test plan** in at most one sentence each. The user will expand
the PR description if more detail is needed. More context:
[AI_POLICY.md](./AI_POLICY.md).

Draft PRs run hygiene, docs, and the Linux x64 EGL/Vulkan targets with their
binding suites. Ready PRs also run macOS Metal, Windows x64 WGL/Vulkan, Android
x64 EGL/Vulkan, and browser WebGL/WebGPU. Within either PR tier, mise's project
graph selects target jobs whose native code or consumer suites are affected.
Each selected job runs all of its suites. Shared root files and native changes
select the complete tier; docs and hygiene always run. Main, manual runs, and
Dependabot-authored PRs run every target and complete packaging verification.

Use persistent PR labels to add coverage to either PR tier:

| Label        | Additional coverage                                               |
| ------------ | ----------------------------------------------------------------- |
| `ci:apple`   | All macOS backends and iOS/tvOS device and simulator targets      |
| `ci:android` | All Android ABIs/backends and multi-ABI packaging                 |
| `ci:linux`   | Linux ARM64 and musl variants                                     |
| `ci:windows` | Windows ARM64 variants                                            |
| `ci:ohos`    | OpenHarmony targets and emulator tests                            |
| `ci:full`    | Every target and complete packaging verification, including Maven |

Labels combine and persist across pushes. Readiness and label changes start a
new run. Explicit platform labels retain all coverage on those platforms even
when the affected graph would omit it. If selection is unavailable, the planner
retains the complete tier and reports the reason. Every job selected by the
planner must succeed for `ci-required` to pass; only jobs omitted by the plan
may be skipped. For CI, ABI, shared toolchain, dependency, or publishing
changes, request full coverage with `gh pr edit <number> --add-label 'ci:full'`.

Keep cross-language dependencies in `[monorepo.projects]` in `mise.toml` when
adding bindings, examples, or generated reference inputs. Cargo path
dependencies are inferred from manifests. `mise run ci:check-project-graph`
verifies that every CI consumer task has project ownership.

## Project Invariants

### General

- Campsite rules apply: leave anything you touch tidier than when you found it.
- Mise defines tools, system packages, and common workflows. CMake presets
  define portable native builds, and Gradle defines Android builds.
- The bindings are meant to be low level and broadly analogous to each other and
  to the C API, exposing MapLibre concepts directly, while following language
  conventions for memory and thread safety. Prioritize safety, similarity, and
  idioms, in that order.
- Bindings expose MapLibre Native concepts directly. Add redundant APIs or
  convenience helpers only when they are strongly justified by target-language
  safety or ergonomics.
- Bindings do not reimplement native validation; they validate binding-owned API
  shape, state, lifetimes, and memory safety.
- We're currently in a prerelease state, so breaking API changes are allowed and
  encouraged over leaving backwards compatibility shims.

### Writing

Follow the `docs-writing` skill in `.agents/skills/` for all prose: the
documentation site, specifications, header comments, and this file. It covers
sentence-level style, page structure, and project terminology.

### Testing

- The bindings tests include broad integration coverage for the C/C++ layer on
  targets where they run.
- For tests that _must_ reach below the bindings, there are dedicated C tests in
  `src/c_api/tests`.
- Each binding's test suite should stand on its own for the C API domains and
  targets it supports, using public binding APIs to validate both native
  workflows and binding-owned safety behavior.
- Avoid trivial tests, tests that verify constants, tests that assert a negative
  (unless valuable), tests that simply test third party code; we want to keep
  our test suite robust and high-value.
- Example apps don't need tests.
- Every test skip should be strictly justified. We do not skip rendering tests
  because the CI environment doesn't support them; we fix the environment.

## Project Docs

Read these docs before changing related code:

- [Binding Specification](docs/src/content/docs/development/binding-specification.md)
  for binding requirements and language binding changes.
- [Map Example Specification](docs/src/content/docs/development/map-example-specification.md)
  for example requirements.
- [Overview](docs/src/content/docs/development/overview.md) for project layout,
  workflow, and tooling.
- [Concepts](docs/src/content/docs/concepts.md) for project scope, ownership,
  threading, events, rendering targets, and host integration boundaries.
- [C API Conventions](docs/src/content/docs/development/c-conventions.md) before
  changing public C headers, C ABI behavior, callbacks, diagnostics, or render
  target contracts.

## External Docs

Read these docs whenever relevant:

- `mise`:
  - <https://mise.jdx.dev/configuration.html>
  - <https://mise.jdx.dev/configuration/settings.html>
  - <https://mise.jdx.dev/configuration/environments.html>
  - <https://mise.jdx.dev/dev-tools/>
  - <https://mise.jdx.dev/environments/>
  - <https://mise.jdx.dev/tasks/>
  - <https://mise.jdx.dev/tasks/toml-tasks.html>
  - <https://mise.jdx.dev/tasks/file-tasks.html>
  - <https://mise.jdx.dev/tasks/task-arguments.html>
  - <https://mise.jdx.dev/tasks/task-configuration.html>
  - ... and many more pages. Browse the site if needed.
- `hk`:
  - <https://hk.jdx.dev/configuration.html>
  - <https://hk.jdx.dev/builtins.html>
  - <https://hk.jdx.dev/reference/examples/>
  - <https://hk.jdx.dev/pkl_introduction.html>
- `dprint`:
  - <https://dprint.dev/config/>
  - <https://dprint.dev/plugins/>
- `vp` / Vite+:
  - <https://viteplus.dev/guide/>
  - <https://viteplus.dev/guide/monorepo>
  - <https://viteplus.dev/guide/lint>
  - <https://viteplus.dev/guide/run>
