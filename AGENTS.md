# AGENTS.md

This project is a C API for MapLibre Native, built for low-level language
bindings and host integrations that need a C boundary instead of direct C++
interop or the popular MapLibre Android/iOS SDKs.

## Project map

- `include/` — Public C API headers (the stable ABI surface).
- `src/` — C++ implementation behind the C headers, plus render backend adapters
  (Vulkan, Metal, OpenGL) and the Zig test support shim.
- `bindings/` — Language bindings (Zig, Rust, Java FFM, Java JNI) that wrap the
  C API in idiomatic target-language interfaces.
- `examples/` — Small demo apps per language/backend (`zig-map`, `rust-map`,
  `zig-readback`, `lwjgl-map`, `swift-map`).
- `third_party/` — Vendored dependencies, primarily the MapLibre Native git
  submodule.
- `docs/` — Astro/Starlight documentation site and generated API reference.

## Workflow

```bash
# Install/refresh all tools, submodules, and dependencies
mise install

# List available tasks across the workspace
mise tasks --all

# Build the native library (also runs configure)
mise run build

# Build and run C API + Zig binding tests (also runs build)
mise run test

# Build and run Rust binding tests (also runs build)
mise run //bindings/rust:ci

# Headless smoke test — no display needed
mise run //examples/zig-readback:run

# GUI map app — use a brief timeout or run in background
mise run //examples/zig-map:run

# Build and test for a different variant (override auto-detected env)
mise -E linux-x64-egl run test

# Run formatters and linters on _all_ files (will stage affected files)
mise run fix

# Run formatters and linters on targeted files (will stage affected files)
hk fix [FILES...]
```

Available mise envs: `linux-x64-vulkan`, `linux-x64-egl`, `linux-arm64-vulkan`,
`linux-arm64-egl`, `macos-arm64-metal`, `macos-arm64-vulkan`, `macos-arm64-egl`,
`windows-x64-vulkan`, `windows-x64-wgl`. The host-matching variant is selected
automatically via `.miserc.toml`.

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

## Project Invariants

### General

- Campsite rules apply: leave anything you touch tidier than when you found it.
- The environment is defined by `mise`; pixi is the current desktop native
  library provider behind mise-managed paths.
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

### Prose

Use positive wording for guidance. Use negative wording for real prohibitions,
safety rules, and hard boundaries.

- Prefer: "Examples stay small and focused."
- Avoid: "Examples should not grow into full applications."
- Prefer: "Higher-level adapters may add execution models above this layer."
- Avoid: "This layer should not try to manage execution models for every
  possible host."

### Specifications

For [specification writing](docs/src/content/docs/development/specifications/):

- Standalone and testable: each requirement should be checkable on its own,
  without pointing at an example or the current tree.
- Add, don’t restate or hedge: link to other docs instead of copying them; avoid
  “or equivalent”, unnamed MAYs, and vague outcomes.
- Scope by constraint: family-wide sections state behavior; platform- or
  API-specific rules belong in clearly labeled subsections.

### Testing

- The Zig bindings tests are also the primary integration test suite for the
  C/C++ layer.
- For tests that _must_ reach below the bindings, there are dedicated tests in
  src/c_api/tests.
- Other bindings (Rust, Java, Swift, etc) should include useful integration
  tests that validate binding behavior through the native C/C++ layer.
  Incidental native coverage from those integration tests is acceptable; chasing
  duplicate full C/C++ coverage in every binding is unnecessary.
- Avoid trivial tests, tests that verify constants, tests that assert a negative
  (unless valuable), tests that simply test third party code; we want to keep
  our test suite robust and high-value.
- Example apps don't need tests.
- Every test skip should be strictly justified. We do not skip rendering tests
  because the CI environment doesn't support them; we fix the environment.

## Project Docs

Read these docs before changing related code:

- [Specifications](docs/src/content/docs/development/specifications/) for
  example and binding requirements.
- [Overview](docs/src/content/docs/development/overview.md) for project layout,
  workflow, and tooling.
- [Concepts](docs/src/content/docs/concepts.md) for project scope, ownership,
  threading, events, rendering targets, and host integration boundaries.
- [C API Conventions](docs/src/content/docs/development/c-conventions.md) before
  changing public C headers, C ABI behavior, callbacks, diagnostics, or render
  target contracts.
- [Binding Conventions](docs/src/content/docs/development/bindings.md) and the
  relevant language-specific binding note in
  `docs/src/content/docs/development/bindings-*.md` before changing a language
  binding or its generated reference docs.

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
- `pixi`:
  - <https://pixi.prefix.dev/latest/reference/pixi_manifest/>
  - <https://pixi.prefix.dev/latest/reference/pixi_configuration/>
