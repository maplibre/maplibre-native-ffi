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

Mise owns tools, system packages, and repository tasks. CMake presets define
native targets and render backends; Gradle owns Android builds.

Use `mise tasks --all` to discover tasks. Common entrypoints are:

```bash
mise run build                       # Build the host native library
mise run test                        # Build and test the C API
mise run //bindings/rust:test         # Build and test one binding
mise run //examples/zig-readback:run   # Headless smoke test
```

See the [development overview](docs/src/content/docs/development/overview.md)
for setup, cross-compilation SDKs, and tooling details.

- Use `mise exec -- ...` for commands outside repository tasks. For read-only
  inspection, use `mise exec --no-deps -- ...` to avoid dependency preparation,
  which can reset managed submodules.
- Select the matching `mise run build <preset>` before trusting target-specific
  clangd diagnostics; the last build selects its compilation database.
- Use a brief timeout or run GUI examples in the background.
- Pre-commit runs formatters and linters. When running `mise run fix` or
  `hk fix [FILES...]` manually, remember that they stage affected files.
- Update cross-language dependencies in `[monorepo.projects]` in `mise.toml`
  when adding bindings, examples, or generated reference inputs.

## Pull requests

Follow the repository PR template and [AI policy](AI_POLICY.md). Keep
**Summary** and **Test plan** to at most one sentence each.

Request `ci:full` coverage for CI, ABI, shared toolchain, dependency, or
publishing changes. Use platform labels when the change needs additional
platform coverage; see
[CI coverage](docs/src/content/docs/development/overview.md#ci-coverage).

## Project invariants

### General

- Campsite rules apply: leave anything you touch tidier than when you found it.
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

## Project docs

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

## External docs

Consult the relevant official docs when changing tool configuration:
[mise](https://mise.jdx.dev/), [hk](https://hk.jdx.dev/),
[dprint](https://dprint.dev/config/), and [Vite+](https://viteplus.dev/guide/).
