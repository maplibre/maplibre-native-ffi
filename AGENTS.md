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
# Install system packages, shared tools, and Git hooks. On Windows, add
# --skip files.
mise bootstrap --yes

mise tasks --all                          # List every task
mise run build                            # Build the host native library
mise run build linux-gnu-x64-egl          # Build another target or backend
mise run test                             # Build and run the C API tests
mise run //bindings/rust:test             # Run one binding's tests
mise run //examples/zig-readback:run      # Headless smoke test
mise run //examples/zig-map:run           # GUI app; use a timeout or background
mise run fix                              # Format and lint every file
hk fix [FILES...]                         # Format and lint some files
```

Project-specific tools install automatically when their tasks run. To run a
command that is not a task, use `mise exec -- ...` so that repository tools are
on the path. Formatters and linters also run on pre-commit.

Clangd uses the compilation database of the last `mise run build <preset>`, so
build the matching preset before trusting target-specific diagnostics.

Android, Emscripten, and OpenHarmony builds need an SDK; see
[Targets that need an SDK](docs/src/content/docs/development/overview.md#targets-that-need-an-sdk).

## Issue and PR organization

Label an issue or a pull request with exactly one kind (`bug`, `feature`,
`perf`, `chore`, or `research`) and at least one `area:*` label. Add `binding:`,
`platform:`, and `renderer:` labels when the work is specific to one, and add
`upstream` when the cause is in MapLibre Native. A patch we carry early still
gets `area:upstream`. Leave GitHub's native issue type unset.

`needs-info` and `do-not-merge` are workflow state, and they do not replace a
kind or an area.

## Pull requests

When you open a pull request, follow the repository PR template and write
**Summary** and **Test plan** in at most one sentence each. The user will expand
the PR description if more detail is needed. More context:
[AI_POLICY.md](./AI_POLICY.md).

Applicable CI jobs are selected automatically. If the selection looks wrong,
trigger a manual run (`gh workflow run CI --ref <branch>`).

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
- [Overview](docs/src/content/docs/development/overview.md) for setup, common
  tasks, and tool ownership.
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
